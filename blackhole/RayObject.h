#ifndef RAYOBJECT_H
#define RAYOBJECT_H

#include "Ray.h"

class Object
{
   public:
      Vec3 pos;
      Material mat;
      //  Constructor
      Object(Vec3 p,Material m)
      {
         pos = p;
         mat = m;
      }
      //  Move sphere
      void move(Vec3 p)
      {
         pos = p;
      }
      //  Intersection
      virtual bool hit(Ray& r)=0;
      virtual Vec3 normal(Vec3& p)=0;
};

#endif
