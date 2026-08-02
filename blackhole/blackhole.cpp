/*
 *  Ray Trace Black Hole
 *
 *  Render a black hole on the CPU or with OpenCL.
 *
 *  Key bindings:
 *  g          Toggle OpenCL/CPU rendering
 *  m          Toggle sphere map
 *  arrows     Change view angle
 *  PgDn/PgUp  Zoom in and out
 *  0          Reset view angle
 *  ESC        Exit
 */

#include "CSCIx239.h"
#include <vector>
#include <string.h>
#include "Ray.h"
#include "RayMarch.h"
#include "InitGPUcl.h"

using namespace std;

//  Global variables
int th=200;                   //  Azimuth of view angle
int ph=0;                   //  Elevation of view angle
int wid,hgt;                //  Screen dimensions
float zoom=3.0;               //  Zoom level
unsigned char* pixels=NULL; //  Pixel array for entire screen
Mat3 rot;                   //  Rotation matrix
vector<Object*> objects;    //  Array of objects

float M = 10.0; // Mass of black hole

//  Sphere map texture data
unsigned char* sphereMap = NULL;
int sphereMapW = 0, sphereMapH = 0;
vector<unsigned char*> sphereMaps;
vector<int> sphereMapWs;
vector<int> sphereMapHs;
vector<const char*> sphereMapNames;
int currentSphereMap = 0;

bool useOpenCL = false;
bool openclReady = false;
RayMarch* openclRayMarch = NULL;
cl_device_id clDevid = 0;
cl_context clContext = NULL;
cl_command_queue clQueue = NULL;
cl_program clProgram = NULL;
cl_kernel clKernel = NULL;
cl_mem clSphereMap = NULL;
cl_mem clPixels = NULL;
int clPixelCount = 0;

static bool UploadOpenCLSphereMap()
{
   if (!clContext || !sphereMap || sphereMapW<1 || sphereMapH<1) return false;
   if (clSphereMap) clReleaseMemObject(clSphereMap);
   cl_int err;
   size_t mapBytes = 3*sphereMapW*sphereMapH;
   clSphereMap = clCreateBuffer(clContext,CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,mapBytes,sphereMap,&err);
   return clSphereMap && !err;
}

static bool ActivateSphereMap(int index)
{
   if (index<0 || index>=(int)sphereMaps.size()) return false;
   sphereMap = sphereMaps[index];
   sphereMapW = sphereMapWs[index];
   sphereMapH = sphereMapHs[index];
   currentSphereMap = index;
   if (openclReady && !UploadOpenCLSphereMap())
   {
      useOpenCL = false;
      openclReady = false;
      fprintf(stderr,"OpenCL sphere map upload failed. Falling back to CPU renderer.\n");
   }
   return true;
}

static void CleanupOpenCL()
{
   if (clPixels) clReleaseMemObject(clPixels);
   if (clSphereMap) clReleaseMemObject(clSphereMap);
   if (clKernel) clReleaseKernel(clKernel);
   if (clProgram) clReleaseProgram(clProgram);
   if (clQueue) clReleaseCommandQueue(clQueue);
   if (clContext) clReleaseContext(clContext);
   clPixels = NULL;
   clSphereMap = NULL;
   clKernel = NULL;
   clProgram = NULL;
   clQueue = NULL;
   clContext = NULL;
   clPixelCount = 0;
   openclReady = false;
   openclRayMarch = NULL;
}

static bool InitOpenCLRenderer()
{
   if (!sphereMap || sphereMapW<1 || sphereMapH<1) return false;
   if (objects.size()!=1) return false;
   RayMarch* marcher = dynamic_cast<RayMarch*>(objects[0]);
   if (!marcher) return false;
   if (InitGPU(0,clDevid,clContext,clQueue)<0) return false;

   cl_int err;
   const char* kernelSource = RayMarchOpenCLKernelSource();
   size_t srcLen = strlen(kernelSource);
   clProgram = clCreateProgramWithSource(clContext,1,&kernelSource,&srcLen,&err);
   if (!clProgram || err) { CleanupOpenCL(); return false; }
   err = clBuildProgram(clProgram,1,&clDevid,NULL,NULL,NULL);
   if (err)
   {
      size_t logSize = 0;
      clGetProgramBuildInfo(clProgram,clDevid,CL_PROGRAM_BUILD_LOG,0,NULL,&logSize);
      if (logSize>1)
      {
         vector<char> log(logSize);
         clGetProgramBuildInfo(clProgram,clDevid,CL_PROGRAM_BUILD_LOG,logSize,log.data(),NULL);
         fprintf(stderr,"OpenCL build log:\n%s\n",log.data());
      }
      CleanupOpenCL();
      return false;
   }
   clKernel = clCreateKernel(clProgram,"RayTraceBH",&err);
   if (!clKernel || err) { CleanupOpenCL(); return false; }

   openclRayMarch = marcher;
   if (!UploadOpenCLSphereMap()) { CleanupOpenCL(); return false; }
   openclReady = true;
   return true;
}

static bool RenderOpenCL()
{
   if (!openclReady || !openclRayMarch) return false;
   cl_int err;
   int pixCount = wid*hgt;
   if (pixCount<1) return false;
   if (!clPixels || clPixelCount!=pixCount)
   {
      if (clPixels) clReleaseMemObject(clPixels);
      clPixels = clCreateBuffer(clContext,CL_MEM_WRITE_ONLY,4*pixCount,NULL,&err);
      if (!clPixels || err) return false;
      clPixelCount = pixCount;
   }

   int maxSteps = openclRayMarch->MaxMarchSteps();
   float dt = openclRayMarch->MarchStep();
   float mass = openclRayMarch->size;
   int arg = 0;
   err  = clSetKernelArg(clKernel,arg++,sizeof(cl_mem),&clPixels);
   err |= clSetKernelArg(clKernel,arg++,sizeof(int),&wid);
   err |= clSetKernelArg(clKernel,arg++,sizeof(int),&hgt);
   err |= clSetKernelArg(clKernel,arg++,sizeof(float),&zoom);
   err |= clSetKernelArg(clKernel,arg++,sizeof(float),&rot.x.x);
   err |= clSetKernelArg(clKernel,arg++,sizeof(float),&rot.x.y);
   err |= clSetKernelArg(clKernel,arg++,sizeof(float),&rot.x.z);
   err |= clSetKernelArg(clKernel,arg++,sizeof(float),&rot.y.x);
   err |= clSetKernelArg(clKernel,arg++,sizeof(float),&rot.y.y);
   err |= clSetKernelArg(clKernel,arg++,sizeof(float),&rot.y.z);
   err |= clSetKernelArg(clKernel,arg++,sizeof(float),&rot.z.x);
   err |= clSetKernelArg(clKernel,arg++,sizeof(float),&rot.z.y);
   err |= clSetKernelArg(clKernel,arg++,sizeof(float),&rot.z.z);
   err |= clSetKernelArg(clKernel,arg++,sizeof(float),&mass);
   err |= clSetKernelArg(clKernel,arg++,sizeof(int),&maxSteps);
   err |= clSetKernelArg(clKernel,arg++,sizeof(float),&dt);
   err |= clSetKernelArg(clKernel,arg++,sizeof(cl_mem),&clSphereMap);
   err |= clSetKernelArg(clKernel,arg++,sizeof(int),&sphereMapW);
   err |= clSetKernelArg(clKernel,arg++,sizeof(int),&sphereMapH);
   if (err) return false;

   size_t global = pixCount;
   err = clEnqueueNDRangeKernel(clQueue,clKernel,1,NULL,&global,NULL,0,NULL,NULL);
   if (err) return false;
   err = clEnqueueReadBuffer(clQueue,clPixels,CL_TRUE,0,4*pixCount,pixels,0,NULL,NULL);
   if (err) return false;
   return true;
}

//  Load a BMP file into a raw pixel buffer (RGB)
void LoadSphereMap(const char* file)
{
   FILE* f = fopen(file, "rb");
   if (!f) Fatal("Cannot open file %s\n", file);
   unsigned short magic;
   if (fread(&magic, 2, 1, f) != 1) Fatal("Cannot read magic from %s\n", file);
   if (magic != 0x4D42 && magic != 0x424D) Fatal("Image magic not BMP in %s\n", file);
   unsigned int dx, dy, off, k;
   unsigned short nbp, bpp;
   if (fseek(f, 8, SEEK_CUR) || fread(&off, 4, 1, f) != 1 ||
       fseek(f, 4, SEEK_CUR) || fread(&dx, 4, 1, f) != 1 || fread(&dy, 4, 1, f) != 1 ||
       fread(&nbp, 2, 1, f) != 1 || fread(&bpp, 2, 1, f) != 1 || fread(&k, 4, 1, f) != 1)
      Fatal("Cannot read header from %s\n", file);
   unsigned int size = 3 * dx * dy;
   unsigned char* map = (unsigned char*)malloc(size);
   if (!map) Fatal("Cannot allocate memory for %s\n", file);
   if (fseek(f, off, SEEK_SET) || fread(map, size, 1, f) != 1)
      Fatal("Error reading data from %s\n", file);
   fclose(f);
   //  BGR -> RGB
   for (unsigned int i = 0; i < size; i += 3)
   {
      unsigned char tmp = map[i];
      map[i] = map[i + 2];
      map[i + 2] = tmp;
   }
   sphereMaps.push_back(map);
   sphereMapWs.push_back(dx);
   sphereMapHs.push_back(dy);
   sphereMapNames.push_back(file);
}

//  Sample the sphere map given a ray direction
Color SampleSphereMap(Vec3 dir)
{
   if (!sphereMap || sphereMapW<1 || sphereMapH<1) return Color(0,0,0);
   dir = normalize(dir);
   float theta = atan2(dir.z, dir.x);
   float phi   = asin(dir.y > 1 ? 1 : (dir.y < -1 ? -1 : dir.y));
   float u = 0.5f + theta / (2.0f * M_PI);
   float v = 0.5f + phi   / M_PI;
   int x = (int)(u * sphereMapW) % sphereMapW;
   int y = (int)(v * sphereMapH) % sphereMapH;
   if (x < 0) x += sphereMapW;
   if (y < 0) y += sphereMapH;
   int idx = 3 * (y * sphereMapW + x);
   return Color(sphereMap[idx] / 255.0f, sphereMap[idx+1] / 255.0f, sphereMap[idx+2] / 255.0f);
}

//
//  Ray trace a pixel
//
Color RayTrace(Vec3 org,Vec3 dir)
{
   Ray ray(org,dir);
   for (unsigned int i=0 ; i<objects.size() ; i++)
   {
      if (objects[i]->hit(ray))
         return Color(0,0,0);
   }
   return SampleSphereMap(ray.dir);
}

//
//  Ray trace a pixels
//
void RayTracePixel(int k)
{
   int i = k%wid;
   int j = k/wid;
   // Perspective projection: all rays originate from eye, fan out through screen pixels
   Vec3 org(0 , 0 , -1000);
   Vec3 dir = normalize(Vec3(zoom*(i-wid/2) , zoom*(j-hgt/2) , (float)wid));
   Color col = RayTrace(rot*org , rot*dir);
   //  Copy color to pixel array
   unsigned char* pix = pixels+4*k;
   *pix++ = col.r>1 ? 255 : (unsigned int)(255*col.r);
   *pix++ = col.g>1 ? 255 : (unsigned int)(255*col.g);
   *pix++ = col.b>1 ? 255 : (unsigned int)(255*col.b);
   *pix++ = 255;
}

//
//  Refresh display
//
void display(GLFWwindow* window)
{
   int k;
   Elapsed();
   //  Ray trace scene
   if (!(useOpenCL && RenderOpenCL()))
   {
      #pragma omp parallel for
      for (k=0;k<hgt*wid;k++)
         RayTracePixel(k);
   }
   //  Time ray tracing
   float t = Elapsed();
   //  Blit scene to screen
   glWindowPos2i(0,0);
   glDrawPixels(wid,hgt,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
   //  Display
   glWindowPos2i(5,5);
   const char* mapName = sphereMapNames.empty() ? "(none)" : sphereMapNames[currentSphereMap];
   Print("Size %dx%d Time %.3fs Mode %s Map %s Angle %d,%d Zoom %.2f",wid,hgt,t,useOpenCL?"OpenCL":"OpenMP",mapName,th,ph,zoom);
   //  Flush
   glFlush();
   glfwSwapBuffers(window);
}

//
//  Set rotation matrix
//
void SetRot(void)
{
   float M[16];
   mat4identity(M);
   mat4rotate(M , ph,1,0,0);
   mat4rotate(M , th,0,1,0);
   //  Copy matrix to row vectors
   rot.x.x = M[0]; rot.x.y = M[4]; rot.x.z = M[8];
   rot.y.x = M[1]; rot.y.y = M[5]; rot.y.z = M[9];
   rot.z.x = M[2]; rot.z.y = M[6]; rot.z.z = M[10];
}

//
//  Key pressed callback
//
void key(GLFWwindow* window,int key,int scancode,int action,int mods)
{
   //  Discard key releases (keeps PRESS and REPEAT)
   if (action==GLFW_RELEASE) return;

   //  Exit on ESC
   if (key == GLFW_KEY_ESCAPE)
      glfwSetWindowShouldClose(window,1);
   //  Reset view angle
   else if (key == GLFW_KEY_0)
      th = ph = 0;
   //  Right arrow key - increase angle by 1 degree
   else if (key == GLFW_KEY_RIGHT)
      th += 1;
   //  Left arrow key - decrease angle by 1 degree
   else if (key == GLFW_KEY_LEFT)
      th -= 1;
   //  Up arrow key - increase elevation by 1 degree
   else if (key == GLFW_KEY_UP)
      ph += 1;
   //  Down arrow key - decrease elevation by 1 degree
   else if (key == GLFW_KEY_DOWN)
      ph -= 1;
   //  Page Up key - increase zoom
   else if (key == GLFW_KEY_PAGE_UP)
   {
      if (zoom > 0.1)
      zoom -= 0.1;
   }
   //  Page Down key - decrease zoom
   else if (key == GLFW_KEY_PAGE_DOWN)
      zoom += 0.1;
   //  Toggle OpenCL rendering
   else if (key == GLFW_KEY_G)
   {
      if (!openclReady)
      {
         openclReady = InitOpenCLRenderer();
         if (!openclReady)
         {
            useOpenCL = false;
            fprintf(stderr,"OpenCL initialization failed. Staying on CPU renderer.\n");
         }
         else
            useOpenCL = true;
      }
      else
         useOpenCL = !useOpenCL;
   }
   //  Cycle sphere map
   else if (key == GLFW_KEY_M && !sphereMaps.empty())
   {
      int nextMap = (currentSphereMap+1)%sphereMaps.size();
      if (ActivateSphereMap(nextMap))
         fprintf(stderr,"Switched sphere map to %s\n",sphereMapNames[currentSphereMap]);
   }

   //  Keep angles to +/-360 degrees
   th %= 360;
   ph %= 360;
   SetRot();
}

//  Window resized callback
//
void reshape(GLFWwindow* window,int width,int height)
{
   //  Get framebuffer dimensions (makes Apple work right)
   glfwGetFramebufferSize(window,&wid,&hgt);
   //  Allocate pixels size of window
   delete pixels;
   pixels = new unsigned char [4*wid*hgt];
   //  Set the viewport to the entire window
   glViewport(0,0, wid,hgt);
}

//
//  Main program with GLFW event loop
//
int main(int argc,char* argv[])
{
   bool requestOpenCL = true;  // Try OpenCL by default
   for (int i=1;i<argc;i++)
   {
      if (!strcmp(argv[i],"--opencl")) requestOpenCL = true;
      if (!strcmp(argv[i],"--cpu")) requestOpenCL = false;
   }

   //  Initialize GLFW
   //GLFWwindow* window = InitWindow("Ray Traced Spheres",0,640,480,&reshape,&key);
   GLFWwindow* window = InitWindow("Ray Traced Black Hole",0,1920,1080,&reshape,&key);

   // Create objects
   objects.push_back(new RayMarch(Vec3(0,  0,0) , M , Material(1,1,1 , 0.0)));  
   //  Load sphere map
   LoadSphereMap("planet.bmp");
   LoadSphereMap("mars.bmp");
   LoadSphereMap("ring.bmp");
   LoadSphereMap("dwarf.bmp");
   ActivateSphereMap(0);
   if (requestOpenCL)
   {
      openclReady = InitOpenCLRenderer();
      useOpenCL = openclReady;
      if (!openclReady)
         fprintf(stderr,"OpenCL unavailable. Falling back to CPU renderer.\n");
   }
   //  Initialize rotation matrix
   SetRot();

   //  Event loop
   ErrCheck("init");
   while(!glfwWindowShouldClose(window))
   {
      //  Display
      display(window);
      //  Wait for events
      glfwWaitEvents();
   }
   //  Shut down GLFW
   CleanupOpenCL();
   for (unsigned int i=0;i<sphereMaps.size();i++)
      free(sphereMaps[i]);
   glfwDestroyWindow(window);
   glfwTerminate();
   return 0;
}
