#ifndef RAY_H
#define RAY_H

#include "Vec.h"

//
//  Ray type
//
class Ray
{
   public:
      Vec3 org;
      Vec3 dir;
      float t;
      Ray(Vec3 o,Vec3 d)
      {
         org = o;
         dir = d;
         t = 1e300;
      }
};

//
//  Material type
//
class Material
{
   public:
      Color col;
      float reflection;
      Material()
      {
         reflection = 0;
      }
      Material(float r,float g,float b,float f)
      {
         col = Color(r,g,b);
         reflection = f;
      }
};

//
//  Light type
//
class Light
{
   public:
      Vec3  pos;
      Color col;
      Light(float x,float y,float z , float r,float g,float b)
      {
         pos = Vec3(x,y,z);
         col = Color(r,g,b);
      }
};

#endif
