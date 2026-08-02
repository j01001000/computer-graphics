#ifndef RAYSPHERE_H
#define RAYSPHERE_H

#include "RayObject.h"

class RaySphere : public Object
{
   public:
      float size;
      //  Constructor
      RaySphere(Vec3 p,float s,Material m):
         Object(p,m)
      {
         size = s;
      }
      //  Intersection
      bool hit(Ray& r);
      Vec3 normal(Vec3& p);
};

#endif
