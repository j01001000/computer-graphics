#include "RayMarch.h"

#include <fstream>
#include <sstream>
#include <string>

const char* RayMarchOpenCLKernelSource()
{
   static std::string source;
   static bool loaded = false;
   if (!loaded)
   {
      std::ifstream in("RayMarchKernel.cl", std::ios::in | std::ios::binary);
      if (in)
      {
         std::ostringstream ss;
         ss << in.rdbuf();
         source = ss.str();
      }
      loaded = true;
   }
   return source.c_str();
}

// computes the cross product of two 3-vectors
Vec3 cross(const Vec3& a, const Vec3& b)
{
   return Vec3(a.y*b.z - a.z*b.y , a.z*b.x - a.x*b.z , a.x*b.y - a.y*b.x);
}

Vec3 accel(const Vec3& pos, const Vec3& vel, float M)
{
   float rd = sqrt(pos*pos);
   Vec3 hh = cross(pos, vel);
   float an = -3.0f * M * (hh*hh) / pow(rd, 5);
   return an * pos;
}

bool RayMarch::hit(Ray& r)
{
   //const float eps = 0.01;
   const float M = size; // mass of the black hole
   const float dt = MarchStep(); // Time step for ray marching

   // Ray marching to find intersection with the sphere
   float t = 0.0;
   Vec3 p = r.org; // initial position of the ray during marching
   while (t < MaxMarchSteps()*dt)
   {
      float rd = sqrt(p*p);
      if (rd > 1001)
         return false; // ray escapes the black hole, return false to indicate it hits the background
      // Update ray position and direction using Runge-Kutta 4th order method
      Vec3 k1v = accel(p, r.dir, M);
      Vec3 k1x = r.dir;
      Vec3 k2v = accel(p + 0.5f*dt*k1x, r.dir + 0.5f*dt*k1v, M);
      Vec3 k2x = r.dir + 0.5f*dt*k1v;
      Vec3 k3v = accel(p + 0.5f*dt*k2x, r.dir + 0.5f*dt*k2v, M);
      Vec3 k3x = r.dir + 0.5f*dt*k2v;
      Vec3 k4v = accel(p + dt*k3x, r.dir + dt*k3v, M);
      Vec3 k4x = r.dir + dt*k3v;
      r.dir += (dt/6.0f) * (k1v + 2.0f*k2v + 2.0f*k3v + k4v);
      p += (dt/6.0f) * (k1x + 2.0f*k2x + 2.0f*k3x + k4x);
      t += dt;

      if (rd < 2*M) // ray is inside the black hole
         return true; // ray hits the black hole, return true to indicate it hits the object
   }
   return false;

}

Vec3 RayMarch::normal(Vec3& p)
{
   return normalize(p-pos);
   //return Vec3(0,0,1);
}
