#ifndef RAYDISC_H
#define RAYDISC_H

#include "RayObject.h"

class RayDisc : public Object
{
   public:
      float size; // size is radius of the disc
      //  Constructor
      RayDisc(Vec3 p,float s,Material m):
         Object(p,m)
      {
         size = s;
      }
      //  Intersection
      bool hit(Ray& r);
      Vec3 normal(Vec3& p);
};

#endif
