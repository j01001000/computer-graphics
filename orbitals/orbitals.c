/*
 *  Particle Shaders
 *
 *  Demonstrate a particle shader using the equations for the hydrogen atomic orbitals.
 *  
 *
 *  Key bindings:
 *  n         Increase energy level
 *  l         Increase angular momentum quantum number
 *  m         Increase magnetic quantum number
 *  p         Toggle between stationary and flickering particles
 *  o         Toggle between particle shaders but there's only one shader
 *  a          Toggle axes
 *  arrows     Change view angle
 *  PgDn/PgUp  Zoom in and out
 *  0          Reset view angle
 *  ESC        Exit
 */
#include "CSCIx239.h"
int axes=1;       //  Display axes
int mode=0;       //  Shader mode
int th=0;         //  Azimuth of view angle
int ph=0;         //  Elevation of view angle
int count;          //  Particle count
double asp=1;     //  Aspect ratio
double dim=4.0;   //  Size of world
#define MODE 1
int shader[MODE] = {0}; //  Shader programs
const char* text[] = {"Hydrogen Atom"};

int particleMode = 0; // 0 for stationary, 1 for flickering

float rScale = 1.0; //  Radial scale
int maxR = 64; //  Max radial steps.
int maxP = 64; //  Max polar angular steps.
int maxA = 64; //  Max azimuthal angular steps.

int n = 1; // Energy level
int l = 0; // Angular momentum quantum number
int m = 0; // Magnetic quantum number

//  Point arrays N = maxR * maxP * maxA
#define N 262144
float Vert[3*N];
float Color[3*N];
float Prob[N];
float Start[N];

//
//  Random numbers with range and offset
//
static float frand(float rng,float off)
{
   return (float)((double)rand()/(double)RAND_MAX*rng+off);
}

// Helper function to calculate the probability amplitude
float probAmp(float r2, float theta)
{
   switch(n) {
      case 1:
         dim = 4.0;
         rScale = 2.5;
         return exp(-0.5*r2);
      case 2:
         if (l == 0) {
            dim = 7.0;
            rScale = 6.0;
            return pow(2 - r2,2)*exp(-0.65*r2);
         }
         if (l == 1) {
            if (m == 0) {
               dim = 7.0;
               rScale = 6.0;
               return pow(r2*Cos(theta),2)*exp(-0.7*r2);
            }
            if (abs(m) == 1) {
               dim = 9.0;
               rScale = 8.0;
               return 2*pow(r2*Sin(theta),2)*exp(-0.7*r2);
            }
         }
      case 3:
         if (l == 0) {
            dim = 15.0;
            rScale = 14.0;
            return pow(27 - 18*r2 + 2*r2*r2,2)*exp(-0.85*r2);
         }
         if (l == 1) {
            dim = 10.0;
            rScale = 9.0;
            if (m == 0) return pow((6 - r2)*r2*Cos(theta),2)*exp(-0.825*r2);
            if (abs(m) == 1) return pow((6 - r2)*r2*Sin(theta),2)*exp(-0.825*r2);
         }
         if (l == 2) {
            dim = 12.0;
            rScale = 11.0;
            if (m == 0) return pow(r2*r2*(3*Cos(theta)*Cos(theta)-1),2)*exp(-1.15*r2);
            if (abs(m) == 1) return pow(r2*r2*Sin(2*theta)/2,2)*exp(-0.85*r2);
            if (abs(m) == 2) return pow(r2*Sin(theta),4)*exp(-0.95*r2);
         }
      case 4:
         if (l == 0) {
            dim = 30.0;
            rScale = 28.0;
            return pow(192-144*r2+24*r2*r2-r2*r2*r2,2)*exp(-0.75*r2);
         }
         if (l == 1) {
            dim = 32.0;
            rScale = 30.0;
            if (m == 0) return pow(r2*(80-20*r2+r2*r2)*Cos(theta),2)*exp(-0.75*r2);
            if (abs(m) == 1) return pow(r2*(80-20*r2+r2*r2)*Sin(theta),2)*exp(-0.75*r2);
         }
         if (l == 2) {
            dim = 36.0;
            rScale = 34.0;
            if (m == 0) return pow((r2*r2*(12-r2))*(3*Cos(theta)*Cos(theta)-1),2)*exp(-0.79*r2);
            if (abs(m) == 1) return pow((r2*r2*(12-r2))*Sin(theta)*Cos(theta),2)*exp(-0.74*r2);
            if (abs(m) == 2) return pow((r2*r2*(12-r2))*Sin(theta)*Sin(theta),2)*exp(-0.74*r2);
         }
         if (l == 3) {
            dim = 30.0;
            rScale = 28.0;
            if (m == 0) return pow(r2*r2*r2*Cos(theta)*(5*Cos(theta)*Cos(theta)-3),2)*exp(-0.92*r2);
            if (abs(m) == 1) return pow(r2*r2*r2*Sin(theta)*(5*Cos(theta)*Cos(theta)-1),2)*exp(-0.88*r2);
            if (abs(m) == 2) return pow(r2*r2*r2*Sin(theta)*Sin(theta)*Cos(theta),2)*exp(-0.88*r2);
            if (abs(m) == 3) return pow(r2*r2*r2*Sin(theta)*Sin(theta)*Sin(theta),2)*exp(-0.88*r2);
         }
   }
   return 1;
}

//
//  Initialize particles
//
void InitPart(void)
{
   //  Array Pointers
   float* vert  = Vert;
   float* color = Color;
   float* prob   = Prob;
   float* start = Start;

   count = maxR * maxP * maxA;

   // Loop over radial steps
   for (float r=0.0;r<maxR;r++)
   {
      // Loop over polar angular steps
      for (float p=0.0;p<maxP;p++)
      {
         // Loop over azimuthal angular steps
         for (float a=0.0;a<maxA;a++)
         {
            // Location x,y,z
            float theta = (p/maxP)*180*frand(0.2,1);
            float phi = (a/maxA)*360*frand(0.2,1);
            float r2 = (r/maxR)*rScale*frand(0.2,1);
            *vert++ = r2*Sin(theta)*Cos(phi);
            *vert++ = r2*Sin(theta)*Sin(phi);
            *vert++ = r2*Cos(theta);
            // Color r,g,b (0.5-1.0)
            *color++ = 1;
            *color++ = 1;
            *color++ = 1;
            // Probability
            *prob++ = probAmp(r2, theta);
            // Launch time
            if (particleMode == 0)
               *start++ = 0;
            else
               *start++ = frand(20.0,0.0);
         }
      }
   }
}

//
//  Draw particles
//
void DrawPart(void)
{
   //  Set particle size
   glPointSize(mode?50:2);
   //  Point vertex location to local array Vert
   glVertexPointer(3,GL_FLOAT,0,Vert);
   //  Point color array to local array Color
   glColorPointer(3,GL_FLOAT,0,Color);
   //  Point attribute arrays to local arrays
   int locprob = glGetAttribLocation(shader[mode],"Prob");
   if (locprob>=0) glVertexAttribPointer(locprob,1,GL_FLOAT,GL_FALSE,0,Prob);
   int locstr = glGetAttribLocation(shader[mode],"Start");
   if (locstr>=0) glVertexAttribPointer(locstr,1,GL_FLOAT,GL_FALSE,0,Start);
   //  Enable arrays used by DrawArrays
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_COLOR_ARRAY);
   if (locprob>=0) glEnableVertexAttribArray(locprob);
   if (locstr>=0) glEnableVertexAttribArray(locstr);
   //  Set transparent large particles
   if (mode)
   {
      glEnable(GL_POINT_SPRITE);
      glTexEnvi(GL_POINT_SPRITE,GL_COORD_REPLACE,GL_TRUE);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA,GL_ONE);
      glDepthMask(0);
   }
   //  Draw arrays
   glDrawArrays(GL_POINTS,0,count);
   //  Reset
   if (mode)
   {
      glDisable(GL_POINT_SPRITE);
      glDisable(GL_BLEND);
      glDepthMask(1);
   }
   //  Disable arrays
   glDisableClientState(GL_VERTEX_ARRAY);
   glDisableClientState(GL_COLOR_ARRAY);
   if (locprob>=0) glDisableVertexAttribArray(locprob);
   if (locstr>=0) glDisableVertexAttribArray(locstr);
}

//
//  Refresh display
//
void display(GLFWwindow* window)
{
   //  Erase the window and the depth buffer
   glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
   //  Enable Z-buffering in OpenGL
   glEnable(GL_DEPTH_TEST);
   //  Undo previous transformations
   View(th,ph,0,dim);

   //
   //  Draw scene
   //
   //  Select shader (0 => no shader)
   glUseProgram(shader[mode]);
   //  Set time
   int id = glGetUniformLocation(shader[mode],"time");
   glUniform1f(id,glfwGetTime());
   id = glGetUniformLocation(shader[mode],"Noise3D");
   glUniform1i(id,1);
   id = glGetUniformLocation(shader[mode],"img");
   glUniform1i(id,0);

   //  Draw the particles
   DrawPart();

   //  No shader for what follows
   glUseProgram(0);

   //  Draw axes - no lighting from here on
   if (axes) Axes(2);
   //  Display parameters
   glWindowPos2i(5,5);
   Print("FPS=%d Dim=%.1f Mode=%s N=%d L=%d M=%d",
     FramesPerSecond(),dim,text[mode],n,l,m);
   //  Render the scene and make it visible
   ErrCheck("display");
   glFlush();
   glfwSwapBuffers(window);
}

//
//  Key pressed callback
//
void key(GLFWwindow* window,int key,int scancode,int action,int mods)
{
   //  Discard key releases (keeps PRESS and REPEAT)
   if (action==GLFW_RELEASE) return;

   //  Check for shift
   int shift = (mods & GLFW_MOD_SHIFT);

   //  Exit on ESC
   if (key == GLFW_KEY_ESCAPE)
      glfwSetWindowShouldClose(window,1);
   //  Reset view angle
   else if (key == GLFW_KEY_0)
      th = ph = 0;
   //  Toggle axes
   else if (key == GLFW_KEY_A)
      axes = 1-axes;
   //  Cycle modes
   else if (key == GLFW_KEY_O)
   {
      mode = shift ? (mode+MODE-1)%MODE : (mode+1)%MODE;
      InitPart();
   }
   // Cycle particle mode
   else if (key == GLFW_KEY_P)
   {
      particleMode = 1 - particleMode;
      InitPart();
   } 
   // Increase energy level
   else if (key == GLFW_KEY_N)
   {
      n = (n % 4) + 1;
      if (n <= l)
         l = 0;
      if (l < abs(m))
         m = 0;
      InitPart();
   }
   // Increase angular momentum quantum number
   else if (key == GLFW_KEY_L)
   {
      l = (l + 1) % n;
      if (l < abs(m))
         m = 0;
      InitPart();
   }
   // Increase magnetic quantum number
   else if (key == GLFW_KEY_M)
   {
      m = (m + l + 1) % (2*l + 1) - l;
      InitPart();
   }
   //  Right arrow key - increase angle by 5 degrees
   else if (key == GLFW_KEY_RIGHT)
      th += 5;
   //  Left arrow key - decrease angle by 5 degrees
   else if (key == GLFW_KEY_LEFT)
      th -= 5;
   //  Up arrow key - increase elevation by 5 degrees
   else if (key == GLFW_KEY_UP)
      ph += 5;
   //  Down arrow key - decrease elevation by 5 degrees
   else if (key == GLFW_KEY_DOWN)
      ph -= 5;
   //  PageUp key - increase dim
   else if (key == GLFW_KEY_PAGE_DOWN || key == GLFW_KEY_MINUS)
      dim += 0.1;
   //  PageDown key - decrease dim
   else if ((key == GLFW_KEY_PAGE_UP || key == GLFW_KEY_EQUAL) && dim>1)
      dim -= 0.1;

   //  Keep angles to +/-360 degrees
   th %= 360;
   ph %= 360;
   //  Update projection
   Projection(0,asp,dim);
}

//
//  Window resized callback
//
void reshape(GLFWwindow* window,int width,int height)
{
   //  Get framebuffer dimensions (makes Apple work right)
   glfwGetFramebufferSize(window,&width,&height);
   //  Ratio of the width to the height of the window
   asp = (height>0) ? (double)width/height : 1;
   //  Set the viewport to the entire window
   glViewport(0,0, width,height);
   //  Set projection
   Projection(0,asp,dim);
}

//
//  Main program with GLFW event loop
//
int main(int argc,char* argv[])
{
   //  Initialize GLFW
   GLFWwindow* window = InitWindow("Particle Shaders",1,800,600,&reshape,&key);

   //  Confetti Cannon needs no fragment shader, but adds Prob and Start
   shader[0] = CreateShaderProg("hydrogen.vert",NULL);
   //shader[1] = CreateShaderProg("fire.vert","fire.frag");
   //  Load random texture
   CreateNoise3D(GL_TEXTURE1);
   //  Load smoke particle
   LoadTexBMP("particle.bmp");
   //  Initialize particles
   InitPart();

   //  Event loop
   ErrCheck("init");
   while(!glfwWindowShouldClose(window))
   {
      //  Display
      display(window);
      //  Process any events
      glfwPollEvents();
   }
   //  Shut down GLFW
   glfwDestroyWindow(window);
   glfwTerminate();
   return 0;
}
