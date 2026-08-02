#include "RayDisc.h"

/*
 *  Check if the ray hits the Disc (contained in the plane z = 0) at positive t less than current value of t
 */
bool RayDisc::hit(Ray& r)
{
   // ray hits disc if z = 0 and x^2+y^2 < size^2
   float vz = r.dir.z;
   // eps is the thickness of the disc.
   const float eps = 0.01;
   // solve for t when ray is at z=0, then check if x^2+y^2 < size^2
   if (fabs(r.org.z) > eps && fabs(vz)<eps)
      return false; // ray is parallel to the disc
   float t0 = -r.org.z / vz;
   if (t0<eps || t0>r.t)
      return false; // ray hits disc behind the ray origin or farther than current hit
   Vec3 p = r.org + t0*r.dir; // ray at plane z = 0
   Vec3 h = pos - p; // vector from ray at plane to disc center
   if (h*h > size*size)
      return false; // ray hits plane outside the disc
   r.t = t0;
   return true;
}

/*
 *  return the normal vector of the disc, which is (0,0,1) since the disc is in the plane z = 0
 */
Vec3 RayDisc::normal(Vec3& p)
{
   return Vec3(0,0,1);
}
