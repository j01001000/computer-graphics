//  Hydrogen atomic orbitals vertex shader
#version 120

uniform   float time;  //  Time
attribute float Start; //  Time offset
attribute float  Prob;   //  Probability

void main(void)
{
   //  Particle location
   vec4 vert = gl_Vertex;
   //  Allows each vertex to pop in and out depending on the randomly assigned frequency Start.
   if (cos(Start*time)>0)
      gl_FrontColor = Prob*vec4(1,1,1,1);
   else
      gl_FrontColor = vec4(0,0,0,0);
   //  Transform particle location
   gl_Position = gl_ModelViewProjectionMatrix*vert;
}
