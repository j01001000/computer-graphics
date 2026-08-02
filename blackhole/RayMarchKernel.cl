#define PI 3.14159265358979323846f

inline float3 cross3(float3 a,float3 b)
{
   return (float3)(a.y*b.z - a.z*b.y , a.z*b.x - a.x*b.z , a.x*b.y - a.y*b.x);
}

inline float3 accel(float3 pos,float3 vel,float mass)
{
   float rd = sqrt(dot(pos,pos));
   float3 h = cross3(pos,vel);
   float an = -3.0f*mass*dot(h,h)/pow(rd,5.0f);
   return an*pos;
}

inline float3 mul3x3(float r00,float r01,float r02,float r10,float r11,float r12,float r20,float r21,float r22,float3 v)
{
   return (float3)(r00*v.x+r01*v.y+r02*v.z, r10*v.x+r11*v.y+r12*v.z, r20*v.x+r21*v.y+r22*v.z);
}

__kernel void RayTraceBH(
   __global uchar4* out,
   int wid,int hgt,float zoom,
   float r00,float r01,float r02,float r10,float r11,float r12,float r20,float r21,float r22,
   float mass,int maxSteps,float dt,
   __global const uchar* map,int mapW,int mapH)
{
   int gid = get_global_id(0);
   int pix = wid*hgt;
   if (gid>=pix) return;

   int i = gid%wid;
   int j = gid/wid;
   float x = zoom*(i-0.5f*(float)wid);
   float y = zoom*(j-0.5f*(float)hgt);

   float3 org = (float3)(0.0f,0.0f,-1000.0f);
   float3 dir = normalize((float3)(x,y,(float)wid));
   float3 p = mul3x3(r00,r01,r02,r10,r11,r12,r20,r21,r22,org);
   float3 d = normalize(mul3x3(r00,r01,r02,r10,r11,r12,r20,r21,r22,dir));

   int captured = 0;
   for (int step=0; step<maxSteps; step++)
   {
      float rd = sqrt(dot(p,p));
      if (rd>1001.0f) break;
      if (rd<2.0f*mass) {captured=1; break;}

      float3 k1v = accel(p,d,mass);
      float3 k1x = d;
      float3 k2v = accel(p+0.5f*dt*k1x,d+0.5f*dt*k1v,mass);
      float3 k2x = d+0.5f*dt*k1v;
      float3 k3v = accel(p+0.5f*dt*k2x,d+0.5f*dt*k2v,mass);
      float3 k3x = d+0.5f*dt*k2v;
      float3 k4v = accel(p+dt*k3x,d+dt*k3v,mass);
      float3 k4x = d+dt*k3v;
      d = normalize(d + (dt/6.0f)*(k1v+2.0f*k2v+2.0f*k3v+k4v));
      p = p + (dt/6.0f)*(k1x+2.0f*k2x+2.0f*k3x+k4x);
   }

   if (captured)
   {
      out[gid] = (uchar4)(0,0,0,255);
      return;
   }

   float theta = atan2(d.z,d.x);
   float phi = asin(clamp(d.y,-1.0f,1.0f));
   float u = 0.5f + theta/(2.0f*PI);
   float v = 0.5f + phi/PI;
   int tx = (int)floor(u*mapW);
   int ty = (int)floor(v*mapH);
   tx = ((tx%mapW)+mapW)%mapW;
   ty = ((ty%mapH)+mapH)%mapH;
   int idx = 3*(ty*mapW+tx);
   out[gid] = (uchar4)(map[idx],map[idx+1],map[idx+2],255);
}
