#ifndef RAYMARCH_H
#define RAYMARCH_H

#include "RayObject.h"

class RayMarch : public Object
{
   public:
      float size;
      static constexpr float kMarchStep = 1.0f;
      // 3000 for RTX 5090
      static constexpr int kMaxMarchSteps = 3000;
      //  Constructor
      RayMarch(Vec3 p,float s,Material m):
         Object(p,m)
      {
         size = s;
      }
      float MarchStep() const { return kMarchStep; }
      int MaxMarchSteps() const { return kMaxMarchSteps; }
      //  Intersection
      bool hit(Ray& r);
      Vec3 normal(Vec3& p);
};

   const char* RayMarchOpenCLKernelSource();

#endif
