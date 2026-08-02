#ifndef VEC_H
#define VEC_H

//  When compiling with CUDA, generate host and device code
#ifdef __CUDACC__
#define CHDM __host__ __device__
#else
#define CHDM
#endif

#include <math.h>
#define Cos(th) cos(3.1415926/180*(th))
#define Sin(th) sin(3.1415926/180*(th))

//
//  2D vector type
//
class Vec2
{
   public:
      float x,y;
      CHDM Vec2(void)
      {
         x = y = 0;
      }
      CHDM Vec2(float X,float Y)
      {
         x = X;
         y = Y;
      }
   CHDM inline Vec2& operator+=(const Vec2& v) {x+=v.x;y+=v.y; return *this;}
};

//
//  3D vector type
//
class Vec3
{
   public:
      float x,y,z;
      CHDM Vec3(void)
      {
         x = y = z = 0;
      }
      CHDM Vec3(float X,float Y,float Z)
      {
         x = X;
         y = Y;
         z = Z;
      }
   CHDM inline Vec3& operator+=(const Vec3& v) {x+=v.x;y+=v.y;z+=v.z; return *this;}
   CHDM inline Vec3  operator-() {return Vec3(-x,-y,-z);}
};
//  3x3 matrix
class Mat3
{
   public:
     Vec3 x,y,z;
};

//  Vec3 operators
CHDM inline Vec3  operator+(const Vec3& v1  , const Vec3& v2) {return Vec3(v1.x+v2.x , v1.y+v2.y , v1.z+v2.z);}  //  v1+v2
CHDM inline Vec3  operator-(const Vec3& v1  , const Vec3& v2) {return Vec3(v1.x-v2.x , v1.y-v2.y , v1.z-v2.z);}  //  v1-v2
CHDM inline float operator*(const Vec3& v1  , const Vec3& v2) {return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z;}        //  v1*v2
CHDM inline Vec3  operator*(float f         , const Vec3& v)  {return Vec3(f*v.x   , f*v.y   , f*v.z);}          //   f*v
CHDM inline Vec3  operator*(const Mat3 rot  , const Vec3& v)  {return Vec3(rot.x*v , rot.y*v , rot.z*v);}        //   Av

//  Normalize Vec3
CHDM inline Vec3 normalize(Vec3 v)
{
   float l = v*v;
   return (l==0) ? Vec3(1,0,0) : (1/sqrtf(l))*v;
}

//  Rotation matrix
CHDM inline Mat3 rotmat(float th,float X,float Y,float Z)
{
   //  Normalize axis
   float l = sqrt(X*X+Y*Y+Z*Z);
   float x = X/l;
   float y = Y/l;
   float z = Z/l;
   //  Calculate sin and cos
   float s = sinf(th*3.1415927/180);
   float c = cosf(th*3.1415927/180);
   float C = 1-c;
   //  Rotation matrix
   Mat3 rot;
   rot.x.x = C*x*x+c;     rot.x.y = C*x*y+z*s;   rot.x.z = C*z*x-y*s;
   rot.y.x = C*x*y-z*s;   rot.y.y = C*y*y+c;     rot.y.z = C*y*z+x*s;
   rot.z.x = C*z*x+y*s;   rot.z.y = C*y*z-x*s;   rot.z.z = C*z*z+c;
   return rot;
}

//
//  Color type
//
struct Color
{
   public:
      float r,g,b,v[4];
      CHDM Color()
      {
         v[0] = v[1] = v[2] = r = g = b = 0;
         v[3] = 1;
      }
      CHDM Color(float R,float G,float B)
      {
         v[0] = r = R;
         v[1] = g = G;
         v[2] = b = B;
         v[3] = 1;
      }
      CHDM Color(const Vec3& V)
      {
         v[0] = r = V.x;
         v[1] = g = V.y;
         v[2] = b = V.z;
         v[3] = 1;
      }
      CHDM Color& operator+=(const Color &c2) {r += c2.r; g += c2.g; b += c2.b; return *this;}
      //  Map color to float*
      CHDM float* fv() {v[0]=r;v[1]=g;v[2]=b;return v;}
};
//  Color operators
CHDM inline Color operator*(const Color& c1 , const Color& c2) {return Color(c1.r*c2.r , c1.g*c2.g , c1.b*c2.b);} // c1*c2
CHDM inline Color operator+(const Color& c1 , const Color& c2) {return Color(c1.r+c2.r , c1.g+c2.g , c1.b+c2.b);} // c1+c2
CHDM inline Color operator*(float f         , const Color& c)  {return Color(f*c.r , f*c.g , f*c.b);}             //  f*c

#endif
