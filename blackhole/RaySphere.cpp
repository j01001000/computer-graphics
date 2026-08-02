#include "RaySphere.h"

/*
 *  Check if the ray hits the Sphere at positive t less than current value of t
 */
bool RaySphere::hit(Ray& r)
{
   const float eps = 0.01;
   Vec3 h = pos - r.org;
   float m = h*r.dir;
   float g = m*m - h*h + size*size;
   if (g<0) return false;
   float t0 = m - sqrt(g);
   float t1 = m + sqrt(g);
   if (t0>eps && t0<r.t)
   {
      r.t = t0;
      return true;
   }
   else if (t1>eps && t1<r.t)
   {
      r.t = t1;
      return true;
   }
   else
      return false;
}

/*
 *  Check if the ray hits the Sphere at positive t less than current value of t
 */
Vec3 RaySphere::normal(Vec3& p)
{
   return normalize(p-pos);
}
