/*
 *  Project: Interstellar - Dr. Miller's Planet
 *
 *  
 *
 *  Key bindings:
 *  l          Toggle lighting on/off
 *  z/Z        Toggle light movement
 *  </>        Move light
 *  r/R        decrease/increase radius of light source
*  t/T        decrease/increase ambient light
 *  y/Y        decrease/increase diffuse light
 *  u/U        decrease/increase specular light
 *  e/E        decrease/increase emitted light
 *  M          reveals incomplete astronaut
 *  c/C        decrease/increase height of wave
 *  m          Toggle between orthogonal, perspective, and first person
 *  +/-        Changes field of view for perspective
 *  w/s/a/d    Move forward, backward, strafe left, and strafe right in first-person mode
 *  n/N        Decrease/increase shininess
 *  []         Lower/rise light
 *  x          Toggle axes
 *  arrows     Change view angle
 *  PgDn/PgUp  Zoom in and out
 *  0          Reset view angle
 *  ESC        Exit
 */
#include "CSCIx229.h"
int move=1;       //  Move light
int mode=1;       //  Projection mode
int axes=0;       //  Display axes
int th=-25;         //  Azimuth of view angle
int ph=15;         //  Elevation of view angle
int fov=45;       //  Field of view (for perspective)
int light=1;      //  Lighting

int reveal = 0; // reveals incomplete astronaut

double radius = 1.0;   // radius of light
double asp=1;     //  Aspect ratio
double dim=6;   //  Size of world
// Light values
int emission  =   50;  // Emission intensity (%)
int ambient   =  50;  // Ambient intensity (%)
int diffuse   = 100;  // Diffuse intensity (%)
int specular  =   0;  // Specular intensity (%)
int shininess =   0;  // Shininess (power of two)
float shiny   =   1;    // Shininess (value)
int zh        =  90;  // Light azimuth
float ylight  =   1.0;  // Elevation of light
unsigned int texture[46]; // Texture names

//  2D vector
typedef struct {double x,y;} vec2;

//  3D vector
typedef struct {double x,y,z;} vec3;

// parameters
double t = 0.0;

// cube map dimension (2*cmsize) x (2*cmsize)
double cmsize = 20;

// thickness of gaussian
double a = 1.5;
// amplitude of wave
double c = 38;

//  Macro for sin & cos in degrees
#define Cos(th) cos(3.14159265/180*(th))
#define Sin(th) sin(3.14159265/180*(th))

// time-step
double dt = 0.2;

// position of the eye (absolute coordinates)
double Ex = 0.0;
double Ey = 0.0;
double Ez = 0.0;

// point of interest (absolute coordinates)
double Cx = 0.0;
double Cy = 0.0;
double Cz = -1.0;

// Up direction
double Ux = 0.0;
double Uy = 1.0;
double Uz = 0.0;

// Look Vector use spherical coordinates wrt the observer.
//double lx = 0.0;
//double ly = 0.0;
//double lz = 0.0;

// Forward vector
double Fx = 0.0;
double Fy = 0.0;
double Fz = 0.0;

// Sideways vector F x U
double Sx = 0.0;
double Sy = 0.0;
double Sz = 0.0;

// Normalizes F
static void normF()
{
    double Flen = sqrt(Fx*Fx+Fy*Fy+Fz*Fz);
    Fx /= Flen;
    Fy /= Flen;
    Fz /= Flen;
}

// Computes and normalizes F then computes S using cross product
static void FS()
{
    Fx = Cx - Ex;
    //Fy = Cy - Ey;
    Fy = 0;
    Fz = Cz - Ez;
    normF();
    Sx = Fy * Uz - Uy * Fz;
    Sy = Fz * Ux - Uz * Fx;
    Sz = Fx * Uy - Ux * Fy;
}

// Gaussian

static double gauss(double x)
{
    return c*exp(-pow(x/a,2));
}

/* Slope of the Gaussian wave at an offset from its peak. */
static double gaussSlope(double x)
{
    return -2*x*gauss(x)/(a*a);
}

/* Quintic easing with zero velocity and acceleration at both ends. */
static double smootherStep(double x)
{
    if (x <= 0) return 0;
    if (x >= 1) return 1;
    return x*x*x*(x*(6*x-15)+10);
}

/*
 * Keep the original flight speed, then smoothly bring it to zero during
 * the last two seconds before touchdown.
 */
static double landingClock(double elapsed)
{
    const double blendStart = 5;
    const double touchdown = 7;

    if (elapsed <= blendStart) return elapsed;
    if (elapsed >= touchdown) return touchdown;

    double s = (elapsed-blendStart)/(touchdown-blendStart);
    double easedTime = s + 4*pow(s,3) - 7*pow(s,4) + 3*pow(s,5);
    return blendStart + (touchdown-blendStart)*easedTime;
}

// Solution n=2, m=1 to the two dimensional wave equation on a square

static double u(double x, double z, double s, double bx, double bz)
{
    // phi_nm
    double phi = 0;
    // nu controls frequency
    double nu = 0.5;
    return 0.1*cos(nu*(4/(bx*bx)+1/(bz*bz))*s+phi)*sin(2*M_PI*x/bx)*sin(M_PI*z/bz);
}

/* Ocean. dx is where the time-independent ocean should be located.
 */

static void ocean(double dx)
{

     //double c = 1/(fabs(a)*sqrt(M_PI));
     double x1 = -cmsize;
     double x2 = -cmsize;
     //double y1 = cmsize;
     //double y2 = cmsize;
     double z1 = 0;
     double z2 = 0;
     
    // dimension of the "box" for the wave equation
    double bx = 2;
     // dimension of small tile
     double d = 0.05;
    // size of ocean
     int scale = floor(cmsize/d);
     
     // vector quantities
     //double length = 1;
     double Nx = 0;
     double Ny = 0;
     double Nz = 0;
     
    //  Save transformation
    glPushMatrix();

     //  Enable textures
     glEnable(GL_TEXTURE_2D);
     glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
     glColor3f(1,1,1);
     glBindTexture(GL_TEXTURE_2D,texture[18]);
     
     // iterate along x axis
    for (int xsteps = -scale; xsteps < scale; xsteps+=1)
     {
         x1 = xsteps*d;
         //y1 = u(x1);
         x2 = x1 + d;
         
         glBegin(GL_QUAD_STRIP);
         //  iterate along z-axis
         for (int zsteps=-scale;zsteps<scale;zsteps+=1)
         {
             z1 = zsteps*d;
             z2 = z1 + d;
            
             double y11 = u(x1-dx, z1, t, bx,bx);
             double y21 = u(x2-dx, z1, t, bx,bx);
             
             double y12 = u(x1-dx, z2, t, bx,bx);
             //y22 = u(x2, z2, t, 1,1);
             
             // first vector is (x1,y11,z1) to (x1,y12 ,z2)
             vec3 v1 = (vec3) {0,y12-y11, z2-z1};
             // second vector is (x1,y11,z1) to (x2,y21 ,z1)
             vec3 v2 = (vec3) {x2-x1, y21-y11, 0};
             
             //P[xsteps+scale][zsteps+scale] = (vec3){x1,y11,z1};
             
             Nx = v1.y*v2.z - v1.z*v2.y;
             Ny = v1.z*v2.x - v1.x*v2.z;
             Nz = v1.x*v2.y - v1.y*v2.x;
             
             double length = sqrt(pow(Nx,2)+pow(Ny,2)+pow(Nz,2));
             
             Nx /= length;
             Ny /= length;
             Nz /= length;
                          
             glColor3f(1,1,1);
             glNormal3f(Nx,Ny,Nz);
             //glNormal3f(N[xsteps+scale][zsteps+scale].x,N[xsteps+scale][zsteps+scale].y,N[xsteps+scale][zsteps+scale].z);
             
             
             // texture x interval: [-cmsize, cmsize], z interval: [-cmsize, cmsize], patch 0.1 x 0.1
             double x1tex = (x1 + cmsize)/(2*cmsize);
             double x2tex = (x2 + cmsize)/(2*cmsize);
             double ztex = (z1 + cmsize)/(2*cmsize);
             double dtex = 0.1/(2*cmsize);
             
             if (zsteps %2 == 0)
                 glTexCoord2f(x1tex,ztex);
             else
                 glTexCoord2f(x2tex,ztex);
             
             glVertex3f(x1,y11,z1);
             
             if (zsteps %2 == 0)
                 glTexCoord2f(x1tex,ztex+dtex);
             else
                 glTexCoord2f(x2tex,ztex+dtex);
             
             glVertex3f(x2,y21,z1);
             
             /*
             if (zsteps %2 == 0)
                 glTexCoord2f(1,1);
             else
                 glTexCoord2f(0,1);
             
             //-0.1 is an offset to render the water below the x-z plane.
             glVertex3f(x1,y11,z1);
             
             if (zsteps %2 == 0)
                 glTexCoord2f(1,0);
             else
             glTexCoord2f(0,0);
             
             //-0.1 is an offset to render the water below the x-z plane.
             glVertex3f(x2,y21,z1);
              */
         }
         glEnd();
     }

     //  Undo transformations and textures
     glPopMatrix();
     glDisable(GL_TEXTURE_2D);
}

/*
 *  Draw a cubemap
 *     dimensions (dx,dy,dz)
 *     rotated th about the y axis
 */
static void cubemap()
{
   //  Set specular color to white
   //float white[] = {1,1,1,1};
   //float Emission[]  = {0.0,0.0,0.01*emission,1.0};
   //glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,shiny);
   //glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,white);
   //glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,Emission);
   //  Save transformation
   glPushMatrix();
   //  Offset, scale and rotate
   glTranslated(0,cmsize,0);
   //glRotated(th,0,1,0);
   glScaled(cmsize,cmsize,cmsize);
   //  Enable textures
   glEnable(GL_TEXTURE_2D);
   glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
   glColor3f(1,1,1);
   //glBindTexture(GL_TEXTURE_2D,texture[0]);
   //  Front
   //glColor3f(1,0,0);
   //glBindTexture(GL_TEXTURE_2D,texture[17]);
   glBindTexture(GL_TEXTURE_2D,texture[13]);
   glBegin(GL_QUADS);
   glNormal3f( 0, 0, -1);
   glTexCoord2f(0,0); glVertex3f(-1,-1, 1);
   glTexCoord2f(1,0); glVertex3f(+1,-1, 1);
   glTexCoord2f(1,1); glVertex3f(+1,+1, 1);
   glTexCoord2f(0,1); glVertex3f(-1,+1, 1);
   glEnd();
   //  Back
   glColor3f(1,1,1);
   //glBindTexture(GL_TEXTURE_2D,texture[17]);
   glBindTexture(GL_TEXTURE_2D,texture[14]);
   glBegin(GL_QUADS);
   glNormal3f( 0, 0,1);
   glTexCoord2f(0,0); glVertex3f(+1,-1,-1);
   glTexCoord2f(1,0); glVertex3f(-1,-1,-1);
   glTexCoord2f(1,1); glVertex3f(-1,+1,-1);
   glTexCoord2f(0,1); glVertex3f(+1,+1,-1);
   glEnd();
   //  Right
   glColor3f(1,1,1);
   //glBindTexture(GL_TEXTURE_2D,texture[17]);
   glBindTexture(GL_TEXTURE_2D,texture[15]);
   glBegin(GL_QUADS);
   glNormal3f(-1, 0, 0);
   glTexCoord2f(0,0); glVertex3f(+1,-1,+1);
   glTexCoord2f(1,0); glVertex3f(+1,-1,-1);
   glTexCoord2f(1,1); glVertex3f(+1,+1,-1);
   glTexCoord2f(0,1); glVertex3f(+1,+1,+1);
   glEnd();
   //  Left
   glColor3f(1,1,1);
   //glBindTexture(GL_TEXTURE_2D,texture[17]);
   glBindTexture(GL_TEXTURE_2D,texture[16]);
   glBegin(GL_QUADS);
   glNormal3f(+1, 0, 0);
   glTexCoord2f(0,0); glVertex3f(-1,-1,-1);
   glTexCoord2f(1,0); glVertex3f(-1,-1,+1);
   glTexCoord2f(1,1); glVertex3f(-1,+1,+1);
   glTexCoord2f(0,1); glVertex3f(-1,+1,-1);
   glEnd();
   //  Top
   glColor3f(1,1,1);
   glBindTexture(GL_TEXTURE_2D,texture[17]);
   glBegin(GL_QUADS);
   glNormal3f( 0,-1, 0);
   glTexCoord2f(0,0); glVertex3f(-1,+1,+1);
   glTexCoord2f(1,0); glVertex3f(+1,+1,+1);
   glTexCoord2f(1,1); glVertex3f(+1,+1,-1);
   glTexCoord2f(0,1); glVertex3f(-1,+1,-1);
   glEnd();
   //  Bottom
    /*
   glColor3f(1,1,1);
   glBindTexture(GL_TEXTURE_2D,texture[18]);
   glBegin(GL_QUADS);
   glNormal3f( 0,+1, 0);
   glTexCoord2f(0,0); glVertex3f(-1,-1,-1);
   glTexCoord2f(1,0); glVertex3f(+1,-1,-1);
   glTexCoord2f(1,1); glVertex3f(+1,-1,+1);
   glTexCoord2f(0,1); glVertex3f(-1,-1,+1);
   glEnd();
     */
   //  Undo transformations and textures
   glPopMatrix();
   glDisable(GL_TEXTURE_2D);
}

/*
 *  Draw ranger
 *    at (x,y,z)
 *    nose towards (dx,dy,dz)
 *    up towards (ux,uy,uz)
 *    size
 */
static void ranger(double x,double y,double z,
                       double dx,double dy,double dz,
                       double ux,double uy, double uz,
                       double s)
{
    
    
   // Dimensions used to size airplane
   //const double wid=0.05;
   //const double nose=+0.50;
   const double cone= 0.20;
   //const double wing= 0.00;
   //const double strk=-0.20;
   //const double tail=-0.50;
   //  Unit vector in direction of flight
   double D0 = sqrt(dx*dx+dy*dy+dz*dz);
   double X0 = dx/D0;
   double Y0 = dy/D0;
   double Z0 = dz/D0;
   //  Unit vector in "up" direction
   double D1 = sqrt(ux*ux+uy*uy+uz*uz);
   double X1 = ux/D1;
   double Y1 = uy/D1;
   double Z1 = uz/D1;
   //  Cross product gives the third vector
   double X2 = Y0*Z1-Y1*Z0;
   double Y2 = Z0*X1-Z1*X0;
   double Z2 = X0*Y1-X1*Y0;
   //  Rotation matrix
   double mat[16];
   mat[0] = X0;   mat[4] = X1;   mat[ 8] = X2;   mat[12] = 0;
   mat[1] = Y0;   mat[5] = Y1;   mat[ 9] = Y2;   mat[13] = 0;
   mat[2] = Z0;   mat[6] = Z1;   mat[10] = Z2;   mat[14] = 0;
   mat[3] =  0;   mat[7] =  0;   mat[11] =  0;   mat[15] = 1;

   //  Save current transforms
   glPushMatrix();
   //  Offset, scale and rotate
   glTranslated(x,y,z);
   glMultMatrixd(mat);
   glScaled(s,s,s);
   glRotatef(-90,1,0,0);
   //  Nose
    
    //  Enable textures
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    glColor3f(1,1,1);
    glBindTexture(GL_TEXTURE_2D,texture[0]);
    
   glPushMatrix();
   glTranslatef(cone,0,0);

    glBegin(GL_QUADS);
    
    //front top
    // (0,3,0) x (7,1.5,-1)
    
    glNormal3f(1,0,7);
    glTexCoord2f(0.95,0.7); glVertex3f(7,1.5,0);
    glTexCoord2f(0.95,.3); glVertex3f(7,-1.5,0);
    glTexCoord2f(0,0.05); glVertex3f(0,-3,1);
    glTexCoord2f(0,0.95); glVertex3f(0,3,1);
    
    glEnd();

    //middle top
    // (0,4,0) x (-3,1,0)
    glColor3f(1,1,1);
    glBindTexture(GL_TEXTURE_2D,texture[1]);
    glBegin(GL_QUADS);

    glNormal3f(0,0,1);
    glTexCoord2f(0.05,0.85); glVertex3f(-3,2,1);
    glTexCoord2f(0.05,0.15); glVertex3f(-3,-2,1);
    glTexCoord2f(0.95,0.05); glVertex3f(0,-3,1);
    glTexCoord2f(0.95,0.95); glVertex3f(0,3,1);
    
    glEnd();
    
    //rear top engine
    // (0,4,0) x (1, 0, 0)
    glColor3f(1,1,1);
    glBindTexture(GL_TEXTURE_2D,texture[2]);
    glBegin(GL_QUADS);
    
    glNormal3f(0,0,1);
    glTexCoord2f(1,1); glVertex3f(-3,2,1); // top right
    glTexCoord2f(1,0); glVertex3f(-3,-2,1); // bot right
    glTexCoord2f(0,0); glVertex3f(-4,-2,1); // bot left
    glTexCoord2f(0,1); glVertex3f(-4,2,1); // top left
    glEnd();
    
    //front bottom
    // (0,3,0) x (7,1.5,0.5)
    
    glColor3f(1,1,1);
    glBindTexture(GL_TEXTURE_2D,texture[4]);
    glBegin(GL_QUADS);
    
    glNormal3f(1.5,0,-21);
    glTexCoord2f(1,0.6); glVertex3f(7,1.5,0); // top right
    glTexCoord2f(0.9,0.2); glVertex3f(7,-1.5,0); // bot right
    glTexCoord2f(0,0); glVertex3f(0,-3,-0.5); // bot left
    glTexCoord2f(0.2,1); glVertex3f(0,3,-0.5); // top left
    
    glEnd();
    
    //rear bottom
    // (0,-4,0) x (-4.5,-1,0.5)
    glColor3f(1,1,1);
    glBindTexture(GL_TEXTURE_2D,texture[5]);
    glBegin(GL_QUADS);
    
    glNormal3f(-1,0,-9);
    glTexCoord2f(0,0.4); glVertex3f(-4.5,-2,0); // left bot
    glTexCoord2f(0.3,0.95); glVertex3f(-4.5,2,0); // left top
    glTexCoord2f(1,0.9); glVertex3f(0,3,-0.5); // right top
    glTexCoord2f(0.75,0); glVertex3f(0,-3,-0.5); // right bot
    
    glEnd();
    
    //exhaust
    // (0,-4,0) x (-0.5,0,-1)
    glColor3f(1,1,1);
    glBindTexture(GL_TEXTURE_2D,texture[3]);
    glBegin(GL_QUADS);
    
    glNormal3f(-2,0,1);
    glTexCoord2f(0.1,0.03); glVertex3f(-4.5,-2,0); // bot left
    glTexCoord2f(0.1,0.97); glVertex3f(-4.5,2,0); // top left
    glTexCoord2f(0.9,0.97); glVertex3f(-4,2,1); // top right
    glTexCoord2f(0.9,0.03); glVertex3f(-4,-2,1); // bottom right
    
   glEnd();

    //side front right
    // (7,1.5,-1) x (0,0,1.5)
    glColor3f(1,1,1);
    glBindTexture(GL_TEXTURE_2D,texture[6]);
    glBegin(GL_TRIANGLES);

    glNormal3f(2.25,-10.5,0);
    glTexCoord2f(1,0.45); glVertex3f(7,-1.5,0); // bot right
    glTexCoord2f(0,1); glVertex3f(0,-3,1); // top left
    glTexCoord2f(0,0); glVertex3f(0,-3,-0.5); // bot left
    
    glEnd();
    
    //side front left
    // (7,-1.5,-1) x (0,0,1.5)
    glColor3f(1,1,1);
    glBindTexture(GL_TEXTURE_2D,texture[7]);
    glBegin(GL_TRIANGLES);
    
    glNormal3f(2.25,10.5,0);
    glTexCoord2f(0,0.45); glVertex3f(7,1.5,0); // bot right
    glTexCoord2f(1,1); glVertex3f(0,3,1); // top left
    glTexCoord2f(1,0); glVertex3f(0,3,-0.5); // bot left
    
    glEnd();
    
    //side middle right
    // (-3,1,0) x (0,0,1.5)
    glColor3f(1,1,1);
    glBindTexture(GL_TEXTURE_2D,texture[8]);
    glBegin(GL_TRIANGLES);
    
    glNormal3f(-1.5,-4.5,0);
    glTexCoord2f(0,1); glVertex3f(-3,-2,1);
    glTexCoord2f(1,1); glVertex3f(0,-3,1);
    glTexCoord2f(1,0); glVertex3f(0,-3,-0.5);
    
    glEnd();
    
    //side middle left
    // (-3,-1,0) x (0,0,1.5)
    glColor3f(1,1,1);
    glBindTexture(GL_TEXTURE_2D,texture[9]);
    glBegin(GL_TRIANGLES);
    
    glNormal3f(-1.5,4.5,0);
    glTexCoord2f(1,1); glVertex3f(-3,2,1);
    glTexCoord2f(0,1); glVertex3f(0,3,1);
    glTexCoord2f(0,0); glVertex3f(0,3,-0.5);
    
    glEnd();
    
    //side back right
    // (3, -1, -1.5) x (1,0,0)
    glColor3f(1,1,1);
    glBindTexture(GL_TEXTURE_2D,texture[10]);
    glBegin(GL_QUADS);
    
    glNormal3f(0,-1.5,1);
    glTexCoord2f(1,0); glVertex3f(0,-3,-0.5); // bot right
    glTexCoord2f(0.35,1); glVertex3f(-3,-2,1); // top right
    glTexCoord2f(0,0.8); glVertex3f(-4,-2,1); // top left
    glTexCoord2f(0,0); glVertex3f(-4.5,-2,0); // bot left
    
    glEnd();
    
    //side back left
    // (3,1,-1.5) x (1,0,0)
    glColor3f(1,1,1);
    glBindTexture(GL_TEXTURE_2D,texture[11]);
    glBegin(GL_QUADS);
    
    glNormal3f(0,1.5,1);
    glTexCoord2f(0,0); glVertex3f(0,3,-0.5); // bot right
    glTexCoord2f(0.65,1); glVertex3f(-3,2,1); // top right
    glTexCoord2f(1,0.8); glVertex3f(-4,2,1); // top left
    glTexCoord2f(1,0); glVertex3f(-4.5,2,0); // bot left
    
    glEnd();
   glPopMatrix();

   //  Undo transformations
   glPopMatrix();
}

/*
 * Draw a wave
 * x is position of the peak
 */
static void wave(double x)
{
   //const int d=2;

    //double c = 1/(fabs(a)*sqrt(M_PI));
    double x1 = -cmsize;
    double x2 = -cmsize;
    double y1 = 2;
    double y2 = 2;
    double z = 0;
    
    // scale
    double d = 1;
    int zscale = floor(cmsize*10);
    
    // vector quantities
    double length = 1;
    double Nx = 0;
    double Ny = 0;
    double Nz = 0;
    
   //  Save transformation
   glPushMatrix();
    
    //glTranslated(x,0,0);
    glTranslated(0,-0.11,0);

    //  Enable textures
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    glColor3f(1,1,1);
    glBindTexture(GL_TEXTURE_2D,texture[18]);
    
    // iterate along x axis
    
    while (x1 < cmsize)
    {
        if (fabs(x1) < 0.25)
            d = 0.005;
        else
            d = 0.1;
        x1 = x2;
        y1 = gauss(x1-x);
        x2 = x1 + d;
        y2 = gauss(x2-x);
        
        length = pow(pow(y1-y2,2)+pow(x2-x1,2),0.5);
        Nx = (y1-y2)/length;
        Ny = (x2-x1)/length;
        
        glBegin(GL_QUAD_STRIP);
        //  iterate along z-axis
        for (int zsteps=-zscale;zsteps<=zscale;zsteps+=1)
        {
            z = zsteps*0.1;
            
            glColor3f(1,1,1);
            glNormal3f(Nx,Ny,Nz);
            
            /*
            if (zsteps %2 == 0)
                glTexCoord2f(1,1);
            else
                glTexCoord2f(0,1);
            
            //-0.1 is an offset to render the water below the x-z plane.
            glVertex3f(x1,y1-0.1,z);
            
            if (zsteps %2 == 0)
                glTexCoord2f(1,0);
            else
            glTexCoord2f(0,0);
            
            //-0.1 is an offset to render the water below the x-z plane.
            glVertex3f(x2,y2-0.1,z);
            */
            
            
            // texture x interval: [-cmsize, cmsize], z interval: [-cmsize, cmsize], patch 0.1 x 0.1
            double x1tex = (x1 + cmsize)/4;
            double x2tex = (x2 + cmsize)/4;
            double ztex = (z + cmsize)/(2*cmsize);
            double dtex = 0.1/(2*cmsize);
            
            if (zsteps %2 == 0)
                glTexCoord2f(x1tex,ztex);
            else
                glTexCoord2f(x2tex,ztex);
            
            glVertex3f(x1,y1,z);
            
            if (zsteps %2 == 0)
                glTexCoord2f(x1tex,ztex+dtex);
            else
                glTexCoord2f(x2tex,ztex+dtex);
            
            glVertex3f(x2,y2,z);
            
        }
        glEnd();
    }

    //  Undo transformations and textures
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
}

// forearm Bezier coordinates

vec2 forearm[4] = {
    (vec2){0.16,-0.49},
    (vec2){0.18,-0.24},
    (vec2){0.19,0.03},
    (vec2){0.17,0.30}
};

vec2 uparm[4] = {
    (vec2){0.12,-0.16},
    (vec2){0.14,0.10},
    (vec2){0.14,0.28},
    (vec2){0.12,0.38}
};

vec2 torso[8] = {
    (vec2){0.7,1},
    (vec2){-0.7,1},
    (vec2){-1.3,0.5},
    (vec2){-1.3,-0.5},
    (vec2){-0.7,-1},
    (vec2){+0.7,-1},
    (vec2){+1.3,-0.5},
    (vec2){+1.3,0.5}
};

vec2 lowleg[4] = {
    (vec2){0.11,-0.41},
    (vec2){0.15,-0.18},
    (vec2){0.17,0.08},
    (vec2){0.18,0.29}
};

vec2 upleg[4] = {
    (vec2){0.16,-0.22},
    (vec2){0.17,-0.06},
    (vec2){0.23,0.13},
    (vec2){0.19,0.58}
};

vec2 foot[5] = {
    (vec2){-1,0},
    (vec2){3,0},
    (vec2){0.9,1},
    (vec2){0.9,2},
    (vec2){-1,2}
};

vec2 hand[6] = {
    (vec2){-0.5,0.6},
    (vec2){-0.5,-0.5},
    (vec2){0.5,-0.5},
    (vec2){0.5,+1},
    (vec2){-1,+1},
    (vec2){-1,0.6}
};

vec2 helmet[4] = {
    (vec2){0.25,-0.28},
    (vec2){0.16,-0.02},
    (vec2){0.44,0.28},
    (vec2){0,0.29}
};

static vec2 bezier(double t, vec2* Pn) {
    // Bernstein polynomials
    double B[] = {pow(1-t,3), 3*t*pow(1-t,2), 3*pow(t,2)*(1-t), pow(t,3)};
    
    double x = 0;
    double y = 0;
    
    for (int i = 0; i < 4; i+=1) {
        x += B[i]*Pn[i].x;
        y += B[i]*Pn[i].y;
    }
    
    return (vec2) {x,y};
}

/*
 *  Draw an 8-sided prism
 *     at (x,y,z)
 *     radius r, height h
 */
static void tprism(double x,double y,double z, double h, double s, double th, double ph, double ps, vec2* Pn, int len)
{

   //  Set specular color to white
   float white[] = {1,1,1,1};
   float Emission[]  = {0.0,0.0,0.01*emission,1.0};
   glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,shiny);
   glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,white);
   glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,Emission);
   //  Save transformation
   glPushMatrix();
   //  Offset, scale and rotate
   glTranslated(x,y,z);
    glRotated(ps,1,0,0);
   glRotated(th,0,1,0);
    glRotated(ph,0,0,1);
   glScaled(s,s,s);
    //glScaled(0.5,0.5,0.5);
   //  Enable textures
   glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
   
    
    glColor3f(1,1,1);
    glBindTexture(GL_TEXTURE_2D,texture[44]);
    
    glBegin(GL_QUAD_STRIP);
    
    // iterate through vertices
    for (int k = 0; k < len/2; k+=1) {
        vec2 P = Pn[k%len];
        vec2 Q = Pn[(k+1)%len];
        
        // Normal vector for P and Q
        double Nx = Q.y - P.y;
        double Ny = P.x - Q.x;
        
        glNormal3f(Nx,Ny,0);
        
        if (k%2 == 0) {
            glTexCoord2f(0,0);
        }
        else {
            glTexCoord2f(1,0);
        }
        glVertex3f(P.x,P.y,0);
        
        if (k%2 == 0) {
            glTexCoord2f(0,1);
        }
        else {
            glTexCoord2f(1,1);
        }
        glVertex3f(P.x,P.y,h);
    }
    
    glEnd();
    
    glColor3f(1,1,1);
    glBindTexture(GL_TEXTURE_2D,texture[43]);
    
    glBegin(GL_QUAD_STRIP);
    
    // iterate through vertices
    for (int k = len/2-1; k < len+1; k+=1) {
        vec2 P = Pn[k%len];
        vec2 Q = Pn[(k+1)%len];
        
        // Normal vector for P and Q
        double Nx = Q.y - P.y;
        double Ny = P.x - Q.x;
        
        glNormal3f(Nx,Ny,0);
        
        if (k%2 == 0) {
            glTexCoord2f(0,0);
        }
        else {
            glTexCoord2f(1,0);
        }
        glVertex3f(P.x,P.y,0);
        
        if (k%2 == 0) {
            glTexCoord2f(0,1);
        }
        else {
            glTexCoord2f(1,1);
        }
        glVertex3f(P.x,P.y,h);
    }
    
    glEnd();
    
    // top lid
    glBegin(GL_POLYGON);
    glNormal3f(0,0,1);
    
    for (int k = 0; k<len; k++) {
        glVertex3f(Pn[k].x,Pn[k].y,h);
    }
    
    glEnd();
    
    // bottom lid
    glBegin(GL_POLYGON);
    glNormal3f(0,0,-1);
    
    for (int k = 0; k<len; k++) {
        glVertex3f(Pn[k].x,Pn[k].y,0);
    }
    
    glEnd();
    
   //  Front
   //if (ntex) glBindTexture(GL_TEXTURE_2D,texture[6]);
   //glBegin(GL_QUADS);
   //glNormal3f( 0,-1, 0);
    
   //glTexCoord2f(0,0); glVertex3f(-1,-1,-1);
   //glTexCoord2f(1,0); glVertex3f(+1,-1,-1);
   //glTexCoord2f(1,1); glVertex3f(+1,-1,+1);
   //glTexCoord2f(0,1); glVertex3f(-1,-1,+1);
   //glEnd();
   //  Undo transformations and textures
   glPopMatrix();
   glDisable(GL_TEXTURE_2D);
}


/*
 *  Draw an 8-sided prism
 *     at (x,y,z)
 *     radius r, height h
 */
static void prism(double x,double y,double z, double h, double s, double th, double ph, double ps, vec2* Pn, int len)
{

   //  Set specular color to white
   float white[] = {1,1,1,1};
   float Emission[]  = {0.0,0.0,0.01*emission,1.0};
   glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,shiny);
   glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,white);
   glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,Emission);
   //  Save transformation
   glPushMatrix();
   //  Offset, scale and rotate
   glTranslated(x,y,z);
    glRotated(ps,1,0,0);
   glRotated(th,0,1,0);
    glRotated(ph,0,0,1);
   glScaled(s,s,s);
    //glScaled(0.5,0.5,0.5);
   //  Enable textures
   //glEnable(GL_TEXTURE_2D);
   //glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
   glColor3f(1,1,1);
   //glBindTexture(GL_TEXTURE_2D,texture[0]);
    
    glBegin(GL_QUAD_STRIP);
    //glBegin(GL_POINTS);
    
    //double dt = 1.0/maxsteps;
    
    // iterate through vertices
    for (int k = 0; k < len+1; k+=1) {
        vec2 P = Pn[k%len];
        vec2 Q = Pn[(k+1)%len];
        
        // Normal vector for P and Q
        double Nx = Q.y - P.y;
        double Ny = P.x - Q.x;
        
        glNormal3f(Nx,Ny,0);
        glVertex3f(P.x,P.y,0);
        glVertex3f(P.x,P.y,h);
    }
    
    glEnd();
    
    // top lid
    glBegin(GL_POLYGON);
    glNormal3f(0,0,1);
    
    for (int k = 0; k<len; k++) {
        glVertex3f(Pn[k].x,Pn[k].y,h);
    }
    
    glEnd();
    
    // bottom lid
    glBegin(GL_POLYGON);
    glNormal3f(0,0,-1);
    
    for (int k = 0; k<len; k++) {
        glVertex3f(Pn[k].x,Pn[k].y,0);
    }
    
    glEnd();
    
   //  Front
   //if (ntex) glBindTexture(GL_TEXTURE_2D,texture[6]);
   //glBegin(GL_QUADS);
   //glNormal3f( 0,-1, 0);
    
   //glTexCoord2f(0,0); glVertex3f(-1,-1,-1);
   //glTexCoord2f(1,0); glVertex3f(+1,-1,-1);
   //glTexCoord2f(1,1); glVertex3f(+1,-1,+1);
   //glTexCoord2f(0,1); glVertex3f(-1,-1,+1);
   //glEnd();
   //  Undo transformations and textures
   glPopMatrix();
   //glDisable(GL_TEXTURE_2D);
}

/*
 *  Draw an arm using bezier curves
 *     at (x,y,z)
 *     radius r, height h
 */
static void helm(double x,double y,double z, double s, double th, double ph, vec2* Pn)
{
    int maxsteps = 10;

   //  Set specular color to white
   float white[] = {1,1,1,1};
   float Emission[]  = {0.0,0.0,0.01*emission,1.0};
   glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,shiny);
   glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,white);
   glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,Emission);
   //  Save transformation
   glPushMatrix();
   //  Offset, scale and rotate
   glTranslated(x,y,z);
   glRotated(th,1,0,0);
    glRotated(ph,0,0,1);
   glScaled(s,s,s);
   //  Enable textures
   glEnable(GL_TEXTURE_2D);
   glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
   glColor3f(1,1,1);
   glBindTexture(GL_TEXTURE_2D,texture[45]);
    
    glBegin(GL_QUAD_STRIP);
       
    double dt = 1.0/maxsteps;
    
    for (int steps = 0; steps < maxsteps; steps+=1) {
        
        vec2 P = bezier(dt*steps, Pn);
        vec2 Q = bezier(dt*(steps+1), Pn);
        
        // Normal vector for P and Q
        double Ny = P.x - Q.x;
        double Nx = Q.y - P.y;
        
        for (int th=0;th<=360;th+=15)
        {
            float c = Cos(th);
            float s = Sin(th);
            
           glNormal3f(Nx*c,Ny,Nx*s);
            glTexCoord2f(P.y,th/360.0); glVertex3f(P.x*c,P.y,P.x*s);
            glTexCoord2f(Q.y,th/360.0); glVertex3f(Q.x*c,Q.y,Q.x*s);
        }
    }
    glEnd();
    
    //top lid
    glBegin(GL_TRIANGLE_FAN);
    vec2 P = bezier(1, Pn);
    glNormal3f(0,1,0);
    glVertex3f(0,P.y,0);
    
    for (int th=0;th<=360;th+=15)
    {
        float c = Cos(th);
        float s = Sin(th);
        
       glVertex3f(P.x*c,P.y,P.x*s);
    }
    
    glEnd();
    
    //bottom lid
    glBegin(GL_TRIANGLE_FAN);
    P = bezier(0, Pn);
    glNormal3f(0,-1,0);
    glVertex3f(0,P.y,0);
    
    for (int th=0;th<=360;th+=15)
    {
        float c = Cos(th);
        float s = Sin(th);
        
       glVertex3f(P.x*c,P.y,P.x*s);
    }
    
    glEnd();
    
   //  Front
   //if (ntex) glBindTexture(GL_TEXTURE_2D,texture[6]);
   //glBegin(GL_QUADS);
   //glNormal3f( 0,-1, 0);
    
   //glTexCoord2f(0,0); glVertex3f(-1,-1,-1);
   //glTexCoord2f(1,0); glVertex3f(+1,-1,-1);
   //glTexCoord2f(1,1); glVertex3f(+1,-1,+1);
   //glTexCoord2f(0,1); glVertex3f(-1,-1,+1);
   //glEnd();
   //  Undo transformations and textures
   glPopMatrix();
   //glDisable(GL_TEXTURE_2D);
}

/*
 *  Draw an arm using bezier curves
 *     at (x,y,z)
 *     radius r, height h
 */
static void bcyl(double x,double y,double z, double s, double th, double ph, vec2* Pn)
{
    int maxsteps = 10;

   //  Set specular color to white
   float white[] = {1,1,1,1};
   float Emission[]  = {0.0,0.0,0.01*emission,1.0};
   glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,shiny);
   glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,white);
   glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,Emission);
   //  Save transformation
   glPushMatrix();
   //  Offset, scale and rotate
   glTranslated(x,y,z);
   glRotated(th,1,0,0);
    glRotated(ph,0,0,1);
   glScaled(s,s,s);
    //glScaled(0.5,0.5,0.5);
   //  Enable textures
   //glEnable(GL_TEXTURE_2D);
   //glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
   glColor3f(1,1,1);
   //glBindTexture(GL_TEXTURE_2D,texture[0]);
    
    glBegin(GL_QUAD_STRIP);
    //glBegin(GL_POINTS);
    
    double dt = 1.0/maxsteps;
    
    for (int steps = 0; steps < maxsteps; steps+=1) {
        
        vec2 P = bezier(dt*steps, Pn);
        vec2 Q = bezier(dt*(steps+1), Pn);
        
        // Normal vector for P and Q
        double Ny = P.x - Q.x;
        double Nx = Q.y - P.y;
        
        for (int th=0;th<=360;th+=15)
        {
            float c = Cos(th);
            float s = Sin(th);
            
           glNormal3f(Nx*c,Ny,Nx*s);
           glVertex3f(P.x*c,P.y,P.x*s);
           glVertex3f(Q.x*c,Q.y,Q.x*s);
        }
    }
    glEnd();
    
    //top lid
    glBegin(GL_TRIANGLE_FAN);
    vec2 P = bezier(1, Pn);
    glNormal3f(0,1,0);
    glVertex3f(0,P.y,0);
    
    for (int th=0;th<=360;th+=15)
    {
        float c = Cos(th);
        float s = Sin(th);
        
       glVertex3f(P.x*c,P.y,P.x*s);
    }
    
    glEnd();
    
    //bottom lid
    glBegin(GL_TRIANGLE_FAN);
    P = bezier(0, Pn);
    glNormal3f(0,-1,0);
    glVertex3f(0,P.y,0);
    
    for (int th=0;th<=360;th+=15)
    {
        float c = Cos(th);
        float s = Sin(th);
        
       glVertex3f(P.x*c,P.y,P.x*s);
    }
    
    glEnd();
    
   //  Front
   //if (ntex) glBindTexture(GL_TEXTURE_2D,texture[6]);
   //glBegin(GL_QUADS);
   //glNormal3f( 0,-1, 0);
    
   //glTexCoord2f(0,0); glVertex3f(-1,-1,-1);
   //glTexCoord2f(1,0); glVertex3f(+1,-1,-1);
   //glTexCoord2f(1,1); glVertex3f(+1,-1,+1);
   //glTexCoord2f(0,1); glVertex3f(-1,-1,+1);
   //glEnd();
   //  Undo transformations and textures
   glPopMatrix();
   //glDisable(GL_TEXTURE_2D);
}

/*
 *  Draw a leg
 *     at (x,y,z)
 *     dimensions (dx,dy,dz)
 *     rotated th about the y axis
 *     rotated ph about the x axis
 */
static void rleg(int legnum, double x,double y,double z,
                 double s,
                 double th, double ph)
{
    //int emission = 0;
    //int shiny = 1;
   //  Set specular color to white
   float white[] = {1,1,1,1};
   float Emission[]  = {0.0,0.0,0.01*emission,1.0};
   glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,shiny);
   glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,white);
   glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,Emission);
   //  Save transformation
   glPushMatrix();
   //  Offset, scale and rotate
   glTranslated(x,y,z);
   glRotated(th,0,1,0);
   glRotated(ph,1,0,0);
   glScaled(s,s,s);
   //  Enable textures
   glEnable(GL_TEXTURE_2D);
   glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,mode?GL_REPLACE:GL_MODULATE);
   //glColor3f(1,1,1);
   //glBindTexture(GL_TEXTURE_2D,texture[0]);
    
   //  Front
   glColor3f(1,1,1);
   glBindTexture(GL_TEXTURE_2D,texture[legnum*6+19]);
   glBegin(GL_QUADS);
   glNormal3f( 0, 0, 1);
   glTexCoord2f(0,0);
    glVertex3f(0, 0, 0);
   glTexCoord2f(1,0);
    glVertex3f(245,0, 0);
   glTexCoord2f(1,1);
    glVertex3f(245,1960, 0);
   glTexCoord2f(0,1);
    glVertex3f(0,1960,0);;
   glEnd();
   //  Back
   glColor3f(1,1,1);
   glBindTexture(GL_TEXTURE_2D,texture[legnum*6+1+19]);
   glBegin(GL_QUADS);
   glNormal3f( 0, 0,-1);
   glTexCoord2f(0,0);
    glVertex3f(245,0,-245);
   glTexCoord2f(1,0);
    glVertex3f(0,0,-245);
   glTexCoord2f(1,1);
    glVertex3f(0,1960,-245);
   glTexCoord2f(0,1);
    glVertex3f(245,1960,-245);
   glEnd();
   //  Right
   glColor3f(1,1,1);
   glBindTexture(GL_TEXTURE_2D,texture[legnum*6+2+19]);
   glBegin(GL_QUADS);
   glNormal3f(+1, 0, 0);
   glTexCoord2f(0,0);
    glVertex3f(245,0,0);
   glTexCoord2f(1,0);
    glVertex3f(245,0,-245);
   glTexCoord2f(1,1);
    glVertex3f(245,1960,-245);
   glTexCoord2f(0,1);
    glVertex3f(245,1960,0);
   glEnd();
   //  Left
   glColor3f(1,1,1);
   glBindTexture(GL_TEXTURE_2D,texture[legnum*6+3+19]);
   glBegin(GL_QUADS);
   glNormal3f(-1, 0, 0);
   glTexCoord2f(0,0);
    glVertex3f(0,0,-245);
   glTexCoord2f(1,0);
    glVertex3f(0,0,0);
   glTexCoord2f(1,1);
    glVertex3f(0,1960,0);
   glTexCoord2f(0,1);
    glVertex3f(0,1960,-245);
   glEnd();
   //  Top
   glColor3f(1,1,1);
   glBindTexture(GL_TEXTURE_2D,texture[legnum*6+4+19]);
   glBegin(GL_QUADS);
   glNormal3f( 0,+1, 0);
   glTexCoord2f(0,0);
    glVertex3f(0,1960,0);
   glTexCoord2f(1,0);
    glVertex3f(245,1960,0);
   glTexCoord2f(1,1);
    glVertex3f(245,1960,-245);
   glTexCoord2f(0,1);
    glVertex3f(0,1960,-245);
   glEnd();
   //  Bottom
   glColor3f(1,1,1);
   glBindTexture(GL_TEXTURE_2D,texture[legnum*6+5+19]);
   glBegin(GL_QUADS);
   glNormal3f( 0,-1, 0);
   glTexCoord2f(0,0);
    glVertex3f(0,0,-245);
   glTexCoord2f(1,0);
    glVertex3f(245,0,-245);
   glTexCoord2f(1,1);
    glVertex3f(245,0,0);
   glTexCoord2f(0,1);
    glVertex3f(0,0,0);
   glEnd();
   //  Undo transformations and textures
   glPopMatrix();
   glDisable(GL_TEXTURE_2D);
}

/*
 * Draw CASE as one rigid object so all four sections follow the same
 * wave height and surface angle.
 */
static void caseRobot(double x,double y,double z,double slope)
{
   glPushMatrix();
   glTranslated(x,y,z);
   glRotated(atan(slope)*180/M_PI,0,0,1);

   rleg(0,-0.375,0.2,0.6,0.001,0,-30);
   rleg(1,-0.125,0.0,0.0,0.001,0,0);
   rleg(2, 0.125,0.0,0.0,0.001,0,0);
   rleg(3, 0.375,0.0,0.3,0.001,0,-10);

   glPopMatrix();
}

/*
 *  Draw a ball
 *     at (x,y,z)
 *     radius r
 */
static void ball(double x,double y,double z,double r)
{
   //  Save transformation
   glPushMatrix();
   //  Offset, scale and rotate
   glTranslated(x,y,z);
   glScaled(r,r,r);
   //  White ball
   glColor3f(1,1,1);
   glutSolidSphere(1.0,16,16);
   //  Undo transofrmations
   glPopMatrix();
}

/*
 *  OpenGL (GLUT) calls this routine to display the scene
 */
void display()
{
    // temporal parameters
    double xh = fmod(t,2.4*cmsize);
    double dx = xh-1.2*cmsize;
    double rangerOffset = -dx;
    double rangerY = gauss(rangerOffset);
    double rangerSlope = gaussSlope(rangerOffset);
    const double caseX = -2.875;
    double caseOffset = caseX-dx;
    double caseY = gauss(caseOffset);
    double caseSlope = gaussSlope(caseOffset);
   //  Erase the window and the depth buffer
   glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
   //  Enable Z-buffering in OpenGL
   glEnable(GL_DEPTH_TEST);
   //  Set perspective
   glLoadIdentity();
    //  Perspective - set eye position
    if (mode == 1)
    {
       Ex = -2*dim*Sin(th)*Cos(ph);
       Ey = +2*dim        *Sin(ph);
       Ez = +2*dim*Cos(th)*Cos(ph);
        Cx = 0;
        Cy = 0;
        Cz = 0;
       gluLookAt(Ex,Ey,Ez , Cx,Cy,Cz , 0,Cos(ph),0);
    }
    //  Orthogonal - set world orientation
    else if (mode == 0)
    {
       glRotatef(ph,1,0,0);
       glRotatef(th,0,1,0);
    }
     // first person
    else
    {
        if (ph >= 90) {
            ph = 90;
        }
        else if (ph <= -90) {
            ph = -90;
        }
        Ey = 1+gauss(Ex-dx);
        Cy = Ey+Sin(ph); //+ Ey;
        Cx = Sin(th)*Cos(ph)+ Ex;
        Cz = -Cos(th)*Cos(ph)+ Ez;
        FS();
        gluLookAt(Ex,Ey,Ez , Cx,Cy,Cz , Ux,Uy,Uz);
    }
    
   //  Light switch
   if (light)
   {
      //  Translate intensity to color vectors
      float Ambient[]   = {0.01*ambient ,0.01*ambient ,0.01*ambient ,1.0};
      float Diffuse[]   = {0.01*diffuse ,0.01*diffuse ,0.01*diffuse ,1.0};
      float Specular[]  = {0.01*specular,0.01*specular,0.01*specular,1.0};
      //  Light direction
      float Position[]  = {radius*Cos(zh),ylight,radius*Sin(zh),1};
      //  Draw light position as ball (still no lighting here)
      glColor3f(1,1,1);
      ball(Position[0],Position[1],Position[2] , 0.1);
      //  OpenGL should normalize normal vectors
      glEnable(GL_NORMALIZE);
      //  Enable lighting
      glEnable(GL_LIGHTING);
      //  glColor sets ambient and diffuse color materials
      glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
      glEnable(GL_COLOR_MATERIAL);
      //  Enable light 0
      glEnable(GL_LIGHT0);
      //  Set ambient, diffuse, specular components and position of light 0
      glLightfv(GL_LIGHT0,GL_AMBIENT ,Ambient);
      glLightfv(GL_LIGHT0,GL_DIFFUSE ,Diffuse);
      glLightfv(GL_LIGHT0,GL_SPECULAR,Specular);
      glLightfv(GL_LIGHT0,GL_POSITION,Position);
   }
   else
      glDisable(GL_LIGHTING);
   //  Draw scene
    
    // draw the cubemap
    // animation
    
    if (reveal == 0) {
        cubemap();
        wave(dx);
        ocean(dx);
        if (xh<7)
        {
            double flightTime = landingClock(xh);
            double landingBlend = smootherStep((xh-5)/2);
            double shipAngle = 90*flightTime;
            double bank = 1-landingBlend;

            ranger(3*Cos(shipAngle),7.5-flightTime,3*Sin(shipAngle),
                   -Sin(shipAngle),0,Cos(shipAngle),
                   -bank*Cos(shipAngle),0.5+0.5*landingBlend,
                   -bank*Sin(shipAngle),0.5);
            
        }
       else
          ranger(0,rangerY+0.5,-3,
                 1,rangerSlope,0, -rangerSlope,1,0, 0.5);
            //ranger(0,0,0,1,0,0,0,1,0,0.2);
           //ranger(0,0,-2,1,0,0,0,1,0,0.2);
           //ranger(0,y,-2,1,dy,0,-dy,1,0,0.05);
       //}
        //else if (5.8 <= xh && xh < 6.2)
        //{
        //    ranger(0,y,-2,Cos((xh-5.8)/0.4*180),Sin((xh-5.8)/0.4*180)*dy,0,-dy,1,0,0.05);
        //}
        //else if (6.2 <= xh)
        //{
        //    ranger(0,y,-2 , 1,dy,0, -dy,1,0, 0.05);
        //}
       
        // Draw CASE riding the wave at its own world position
        caseRobot(caseX,caseY,0,caseSlope);
    }
    
    else
    {
        //Draw incomplete astronaut
        // helmet
        helm(0,2.0,0,2,0,0,helmet);
        
        // leftt hand
        prism(1.1,-0.3,0.5,0.5,0.3,90,0,-25,hand,6);
        
        // right hand
        prism(-1.24,-0.3,0.5,0.5,0.3,90,0,-25,hand,6);
        
        // right foot
        prism(-0.4,-2.75,0.05,1.5,0.2,-95,0,5,foot,5);
        
        // left foot
        prism(+0.7,-2.75,0.05,1.5,0.2,-85,0,5,foot,5);
        
        // left arm
        bcyl(1.12,0.5,0.12,1.2,-25,5,forearm);
        bcyl(1,1,0,1.85,0,20,uparm);
        
        // right arm
        bcyl(-1.12,0.5,0.12,1.2,-25,-5,forearm);
        bcyl(-1,1,0,1.85,0,-20,uparm);
        
        // torso
        tprism(0,0,0,3,0.5,0,0,-90,torso,8);
        
        // lower left leg
        bcyl(0.50,-1.8,0.05,1.5,0,5,lowleg);
        
        // upper left leg
        bcyl(0.42,-1,0,1.8,-5,5,upleg);
        
        // lower right leg
        bcyl(-0.50,-1.8,0.05,1.5,0,-5,lowleg);
        
        // right leg
        bcyl(-0.42,-1,0,1.8,-5,-5,upleg);
    }
    
    
   //  Draw axes - no lighting from here on
   glDisable(GL_LIGHTING);
   glColor3f(1,1,1);
   if (axes)
   {
      const double len=2.0; //  Length of axes
      glBegin(GL_LINES);
      glVertex3d(0.0,0.0,0.0);
      glVertex3d(len,0.0,0.0);
      glVertex3d(0.0,0.0,0.0);
      glVertex3d(0.0,len,0.0);
      glVertex3d(0.0,0.0,0.0);
      glVertex3d(0.0,0.0,len);
      glEnd();
      //  Label axes
      glRasterPos3d(len,0.0,0.0);
      Print("X");
      glRasterPos3d(0.0,len,0.0);
      Print("Y");
      glRasterPos3d(0.0,0.0,len);
      Print("Z");
   }
   //  Display parameters
   glWindowPos2i(5,5);
    Print("Angle=%d,%d  Dim=%.1f Light=%s \n",th,ph,dim,light?"On":"Off");
    Print("Tide=%.1f \n",c);
    if (mode == 2){
        Print("FOV=%d Projection=First Person (x,y,z)=(%.1f,%.1f,%.1f) \n",fov,Ex,Ey,Ez);
        Print("Forward=(%.2f,%.2f,%.2f) \n", Fx,Fy,Fz);
    }
    else
        Print("FOV=%d Projection=%s \n",fov,mode?"Perpective":"Orthogonal");
   if (light)
   {
      glWindowPos2i(5,25);
      Print("Ambient=%d  Diffuse=%d Specular=%d Emission=%d Shininess=%.0f \n",ambient,diffuse,specular,emission,shiny);
      Print("Light Radius=%.1f Light Elevation=%.1f",radius, ylight);
   }
   //  Render the scene and make it visible
   ErrCheck("display");
   glFlush();
   glutSwapBuffers();
}

/*
 *  GLUT calls this routine when the window is resized
 */
void idle()
{
   //  Elapsed time in seconds
   t = glutGet(GLUT_ELAPSED_TIME)/1000.0;
   zh = fmod(90*t,360.0);
   //  Tell GLUT it is necessary to redisplay the scene
   glutPostRedisplay();
}

/*
 *  GLUT calls this routine when an arrow key is pressed
 */
void special(int key,int x,int y)
{
   //  Right arrow key - increase angle by 5 degrees
    if (key == GLUT_KEY_RIGHT) {
            th += 5;
    }
   //  Left arrow key - decrease angle by 5 degrees
    else if (key == GLUT_KEY_LEFT) {
           th -= 5;
    }
   //  Up arrow key - increase elevation by 5 degrees
   else if (key == GLUT_KEY_UP)
           ph += 5;
   //  Down arrow key - decrease elevation by 5 degrees
   else if (key == GLUT_KEY_DOWN)
           ph -= 5;
   //  PageUp key - increase dim
   else if (key == GLUT_KEY_PAGE_DOWN)
      dim += 0.1;
   //  PageDown key - decrease dim
   else if (key == GLUT_KEY_PAGE_UP && dim>1)
      dim -= 0.1;
   //  Keep angles to +/-360 degrees
   th %= 360;
   ph %= 360;
   //  Update projection
   Project(mode? fov : 0,asp,dim);
   //  Tell GLUT it is necessary to redisplay the scene
   glutPostRedisplay();
}

/*
 *  GLUT calls this routine when a key is pressed
 */
void key(unsigned char ch,int x,int y)
{
   //  Exit on ESC
   if (ch == 27)
      exit(0);
   //  Reset view angle
   else if (ch == '0')
      th = ph = 0;
   //  Toggle texture mode
   else if (ch == 'm')
       mode = (mode + 1)%3;
   //  Toggle axes
   else if (ch == 'x' || ch == 'X')
      axes = 1-axes;
   //  Toggle lighting
   else if (ch == 'l' || ch == 'L')
      light = 1-light;
    //  Toggle light movement
   else if (ch == 'z' || ch == 'Z')
       move = 1-move;
   //  Toggle textures mode
   //else if (ch == 't')
   //   ntex = 1-ntex;
    
    //  reveal astronaut
    else if (ch == 'M')
       reveal = 1 - reveal;
    
    //  Move light
    else if (ch == '<')
       zh += 1;
    else if (ch == '>')
       zh -= 1;
   //  Light elevation
   else if (ch=='[')
      ylight -= 0.1;
   else if (ch==']')
      ylight += 0.1;
    //  Light radius
    else if (ch=='r')
       radius -= 0.2;
    else if (ch=='R')
       radius += 0.2;
    //  height of wave
    else if (ch=='c')
       c -= 1;
    else if (ch=='C')
        c += 1;
   //  Ambient level
   else if (ch=='t' && ambient>0)
      ambient -= 5;
   else if (ch=='T' && ambient<100)
      ambient += 5;
   //  Diffuse level
   else if (ch=='y' && diffuse>0)
      diffuse -= 5;
   else if (ch=='Y' && diffuse<100)
      diffuse += 5;
   //  Specular level
   else if (ch=='u' && specular>0)
      specular -= 5;
   else if (ch=='U' && specular<100)
      specular += 5;
   //  Emission level
   else if (ch=='e' && emission>0)
      emission -= 5;
   else if (ch=='E' && emission<100)
      emission += 5;
   //  Shininess level
   else if (ch=='n' && shininess>-1)
      shininess -= 1;
   else if (ch=='N' && shininess<7)
      shininess += 1;
    //  Change field of view angle
    else if (ch == '-' && ch>1)
       fov--;
    else if (ch == '+' && ch<179)
       fov++;
     // move forward in first-person
    else if (mode == 2 && ch == 'w') {
        Ex += dt * Fx;
        Ey += dt * Fy;
        Ez += dt * Fz;
        Cx += dt * Fx;
        Cy += dt * Fy;
        Cz += dt * Fz;
    }
     // move back in first person
    else if (mode == 2 && ch == 's') {
        Ex -= dt * Fx;
        Ey -= dt * Fy;
        Ez -= dt * Fz;
        Cx -= dt * Fx;
        Cy -= dt * Fy;
        Cz -= dt * Fz;
    }
     // strafe left in first person
    else if (mode == 2 && ch == 'a') {
        Ex -= dt * Sx;
        Ey -= dt * Sy;
        Ez -= dt * Sz;
        Cx -= dt * Sx;
        Cy -= dt * Sy;
        Cz -= dt * Sz;
    }
     // strafe right in first person
    else if (mode == 2 && ch == 'd') {
        Ex += dt * Sx;
        Ey += dt * Sy;
        Ez += dt * Sz;
        Cx += dt * Sx;
        Cy += dt * Sy;
        Cz += dt * Sz;
    }
   //  Translate shininess power to value (-1 => 0)
   shiny = shininess<0 ? 0 : pow(2.0,shininess);
    //  Animate if requested
    glutIdleFunc(move?idle:NULL);
   //  Reproject
    Project(mode? fov : 0,asp,dim);
   //  Tell GLUT it is necessary to redisplay the scene
   glutPostRedisplay();
}

/*
 *  GLUT calls this routine when the window is resized
 */
void reshape(int width,int height)
{
   //  Ratio of the width to the height of the window
   asp = (height>0) ? (double)width/height : 1;
   //  Set the viewport to the entire window
   glViewport(0,0, RES*width,RES*height);
   //  Set projection
   Project(mode? fov : 0,asp,dim);
}

/*
 *  Start up GLUT and tell it what to do
 */
int main(int argc,char* argv[])
{
   //  Initialize GLUT
   glutInit(&argc,argv);
   //  Request double buffered, true color window with Z buffering at 600x600
   glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
   glutInitWindowSize(1000,1000);
   glutCreateWindow("Jun Hong");
#ifdef USEGLEW
   //  Initialize GLEW
   if (glewInit()!=GLEW_OK) Fatal("Error initializing GLEW\n");
#endif
   //  Set callbacks
   glutDisplayFunc(display);
   glutReshapeFunc(reshape);
   glutSpecialFunc(special);
   glutKeyboardFunc(key);
   glutIdleFunc(idle);
   //  Load textures
   //texture[0] = LoadTexBMP("crate.bmp");
   texture[0] = LoadTexBMP("test.bmp");
   texture[1] = LoadTexBMP("toprear.bmp");
   texture[2] = LoadTexBMP("topengine.bmp");
   texture[3] = LoadTexBMP("exhaust.bmp");
   texture[4] = LoadTexBMP("underside.bmp");
   texture[5] = LoadTexBMP("botrear.bmp");
   texture[6] = LoadTexBMP("siderightfront.bmp");
   texture[7] = LoadTexBMP("sideleftfront.bmp");
   texture[8] = LoadTexBMP("siderightmid.bmp");
   texture[9] = LoadTexBMP("sideleftmid.bmp");
   texture[10] = LoadTexBMP("sidebackright.bmp");
   texture[11] = LoadTexBMP("sidebackleft.bmp");
   texture[12] = LoadTexBMP("water.bmp");
    
    texture[13] = LoadTexBMP("front.bmp");
    texture[14] = LoadTexBMP("back.bmp");
    texture[15] = LoadTexBMP("right.bmp");
    texture[16] = LoadTexBMP("left.bmp");
    texture[17] = LoadTexBMP("top.bmp");
    texture[18] = LoadTexBMP("bottom.bmp");
    
    //CASE starts at 19
    texture[19] = LoadTexBMP("leg1front.bmp");
    texture[20] = LoadTexBMP("leg1back.bmp");
    texture[21] = LoadTexBMP("leg1right.bmp");
    texture[22] = LoadTexBMP("leg1left.bmp");
    texture[23] = LoadTexBMP("leg1top.bmp");
    texture[24] = LoadTexBMP("leg1bot.bmp");
     
     texture[25] = LoadTexBMP("leg2front.bmp");
     texture[26] = LoadTexBMP("leg2back.bmp");
     texture[27] = LoadTexBMP("leg2right.bmp");
     texture[28] = LoadTexBMP("leg2left.bmp");
     texture[29] = LoadTexBMP("leg2top.bmp");
     texture[30] = LoadTexBMP("leg2bot.bmp");
     
     texture[31] = LoadTexBMP("leg3front.bmp");
     texture[32] = LoadTexBMP("leg3back.bmp");
     texture[33] = LoadTexBMP("leg3right.bmp");
     texture[34] = LoadTexBMP("leg3left.bmp");
     texture[35] = LoadTexBMP("leg3top.bmp");
     texture[36] = LoadTexBMP("leg3bot.bmp");
     
     texture[37] = LoadTexBMP("leg4front.bmp");
     texture[38] = LoadTexBMP("leg4back.bmp");
     texture[39] = LoadTexBMP("leg4right.bmp");
     texture[40] = LoadTexBMP("leg4left.bmp");
     texture[41] = LoadTexBMP("leg4top.bmp");
     texture[42] = LoadTexBMP("leg4bot.bmp");
     
     texture[43] = LoadTexBMP("frontside.bmp");
     texture[44] = LoadTexBMP("backside.bmp");
     texture[45] = LoadTexBMP("helmet.bmp");
    
    //texture[19] = LoadTexBMP("water2.bmp");
    
   //  Pass control to GLUT so it can interact with the user
   ErrCheck("init");
   glutMainLoop();
   return 0;
}
