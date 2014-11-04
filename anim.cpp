////////////////////////////////////////////////////
// anim.cpp version 4.1
// Template code for drawing an articulated figure.
// CS 174A 
////////////////////////////////////////////////////

#ifdef _WIN32
#include <windows.h>
#include "GL/glew.h"
#include <GL/gl.h>
#include <GL/glu.h>
#else
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#ifdef _WIN32
#include "GL/freeglut.h"
#else
#include <GLUT/glut.h>
#endif

#include "Ball.h"
#include "FrameSaver.h"
#include "Timer.h"
#include "Shapes.h"
#include "tga.h"

#include "Angel/Angel.h"

#ifdef __APPLE__
#define glutInitContextVersion(a,b)
#define glutInitContextProfile(a)
#define glewExperimental int glewExperimentalAPPLE
#define glewInit()
#endif

/////////////////////////////////////////////////////
// These are global variables
//
//
// Add additional ones if you need,
// especially for animation
//////////////////////////////////////////////////////

FrameSaver FrSaver ;
Timer TM ;

BallData *Arcball = NULL ;
int Width = 480;
int Height = 480 ;
int Button = -1 ;
float Zoom = 1 ;
int PrevY = 0 ;

int Animate = 0 ;
int Recording = 0 ;

void resetArcball() ;
void save_image();
void instructions();
void set_colour(float r, float g, float b) ;

const int STRLEN = 100;
typedef char STR[STRLEN];

#define PI 3.1415926535897
#define X 0
#define Y 1
#define Z 2

//texture
GLuint texture_cube;
GLuint texture_earth;

// Structs that hold the Vertex Array Object index and number of vertices of each shape.
ShapeData cubeData;
ShapeData sphereData;
ShapeData coneData;
ShapeData cylData;

// Matrix stack that can be used to push and pop the modelview matrix.
class MatrixStack {
    int    _index;
    int    _size;
    mat4*  _matrices;

   public:
    MatrixStack( int numMatrices = 32 ):_index(0), _size(numMatrices)
        { _matrices = new mat4[numMatrices]; }

    ~MatrixStack()
	{ delete[]_matrices; }

    void push( const mat4& m ) {
        assert( _index + 1 < _size );
        _matrices[_index++] = m;
    }

    mat4& pop( void ) {
        assert( _index - 1 >= 0 );
        _index--;
        return _matrices[_index];
    }
};

MatrixStack  mvstack;
mat4         model_view;
GLint        uModelView, uProjection, uView;
GLint        uAmbient, uDiffuse, uSpecular, uLightPos, uShininess;
GLint        uTex, uEnableTex;

// The eye point and look-at point.
// Currently unused. Use to control a camera with LookAt().
Angel::vec4 eye(0, 0.0, 50.0,1.0);
Angel::vec4 ref(0.0, 0.0, 0.0,1.0);
Angel::vec4 up(0.0,1.0,0.0,0.0);

double TIME = 0.0 ;

mat4 model_trans;

float tx = -23;
float ty = 26;
float tz = -3.5;
float m_angle = 0;
float facedownAngle = 0;
float rotatearmAngle = 0;

bool impact = false;
float spreadx1 = 0;
float spreadz1 = 0;
float spreadx2 = 0;
float spreadz2 = 0;

/////////////////////////////////////////////////////
//    PROC: drawCylinder()
//    DOES: this function 
//          render a solid cylinder  oriented along the Z axis. Both bases are of radius 1. 
//          The bases of the cylinder are placed at Z = 0, and at Z = 1.
//
//          
// Don't change.
//////////////////////////////////////////////////////
void drawCylinder(void)
{
    glUniformMatrix4fv( uModelView, 1, GL_TRUE, model_view );
    glBindVertexArray( cylData.vao );
    glDrawArrays( GL_TRIANGLES, 0, cylData.numVertices );
}


//////////////////////////////////////////////////////
//    PROC: drawCone()
//    DOES: this function 
//          render a solid cone oriented along the Z axis with base radius 1. 
//          The base of the cone is placed at Z = 0, and the top at Z = 1. 
//         
// Don't change.
//////////////////////////////////////////////////////
void drawCone(void)
{
    glUniformMatrix4fv( uModelView, 1, GL_TRUE, model_view );
    glBindVertexArray( coneData.vao );
    glDrawArrays( GL_TRIANGLES, 0, coneData.numVertices );
}


//////////////////////////////////////////////////////
//    PROC: drawCube()
//    DOES: this function draws a cube with dimensions 1,1,1
//          centered around the origin.
// 
// Don't change.
//////////////////////////////////////////////////////
void drawCube(void)
{
    glBindTexture( GL_TEXTURE_2D, texture_cube );
    glUniformMatrix4fv( uModelView, 1, GL_TRUE, model_view );
    glBindVertexArray( cubeData.vao );
    glDrawArrays( GL_TRIANGLES, 0, cubeData.numVertices );
}


//////////////////////////////////////////////////////
//    PROC: drawSphere()
//    DOES: this function draws a sphere with radius 1
//          centered around the origin.
// 
// Don't change.
//////////////////////////////////////////////////////
void drawSphere(void)
{
    glBindTexture( GL_TEXTURE_2D, texture_earth);
    glUniformMatrix4fv( uModelView, 1, GL_TRUE, model_view );
    glBindVertexArray( sphereData.vao );
    glDrawArrays( GL_TRIANGLES, 0, sphereData.numVertices );
}


void resetArcball()
{
    Ball_Init(Arcball);
    Ball_Place(Arcball,qOne,0.75);
}


/*********************************************************
 PROC: set_colour();
 DOES: sets all material properties to the given colour
 -- don't change
 **********************************************************/

void set_colour(float r, float g, float b)
{
    float ambient  = 0.2f;
    float diffuse  = 0.6f;
    float specular = 0.2f;
    glUniform4f(uAmbient,  ambient*r,  ambient*g,  ambient*b,  1.0f);
    glUniform4f(uDiffuse,  diffuse*r,  diffuse*g,  diffuse*b,  1.0f);
    glUniform4f(uSpecular, specular*r, specular*g, specular*b, 1.0f);
}


/*********************************************************
 PROC: instructions()
 DOES: display instruction in the console window.
 -- No need to change

 **********************************************************/
void instructions()
{
    printf("Press:\n");
    printf("  s to save the image\n");
    printf("  r to restore the original view.\n") ;
    printf("  0 to set it to the zero state.\n") ;
    printf("  a to toggle the animation.\n") ;
    printf("  m to toggle frame dumping.\n") ;
    printf("  q to quit.\n");
}


/*********************************************************
 PROC: myinit()
 DOES: performs most of the OpenGL intialization
 -- change these with care, if you must.
 
 **********************************************************/
void myinit(void)
{
    // Load shaders and use the resulting shader program
    GLuint program = InitShader( "vshader.glsl", "fshader.glsl" );
    glUseProgram(program);
    
    // Generate vertex arrays for geometric shapes
    generateCube(program, &cubeData);
    generateSphere(program, &sphereData);
    generateCone(program, &coneData);
    generateCylinder(program, &cylData);
    
    uModelView  = glGetUniformLocation( program, "ModelView"  );
    uProjection = glGetUniformLocation( program, "Projection" );
    uView       = glGetUniformLocation( program, "View"       );
    
    glClearColor( 0.1, 0.1, 0.2, 1.0 ); // dark blue background
    
    uAmbient   = glGetUniformLocation( program, "AmbientProduct"  );
    uDiffuse   = glGetUniformLocation( program, "DiffuseProduct"  );
    uSpecular  = glGetUniformLocation( program, "SpecularProduct" );
    uLightPos  = glGetUniformLocation( program, "LightPosition"   );
    uShininess = glGetUniformLocation( program, "Shininess"       );
    uTex       = glGetUniformLocation( program, "Tex"             );
    uEnableTex = glGetUniformLocation( program, "EnableTex"       );
    
    glUniform4f(uAmbient,    0.2f,  0.2f,  0.2f, 1.0f);
    glUniform4f(uDiffuse,    0.6f,  0.6f,  0.6f, 1.0f);
    glUniform4f(uSpecular,   0.2f,  0.2f,  0.2f, 1.0f);
    glUniform4f(uLightPos,  15.0f, 15.0f, 30.0f, 0.0f);
    glUniform1f(uShininess, 100.0f);
    
    glEnable(GL_DEPTH_TEST);
    
    Arcball = new BallData;
    Ball_Init(Arcball);
    Ball_Place(Arcball,qOne,0.75);
}


//////////////////////////////////////////////////////
//    PROC: myKey()
//    DOES: this function gets caled for any keypresses
// 
//////////////////////////////////////////////////////
void myKey(unsigned char key, int x, int y)
{
    float time ;
    switch (key) {
        case 'q':
        case 27:
            exit(0); 
        case 's':
            FrSaver.DumpPPM(Width,Height) ;
            break;
        case 'r':
            resetArcball() ;
            glutPostRedisplay() ;
            break ;
        case 'a': // togle animation
            Animate = 1 - Animate ;
            // reset the timer to point to the current time		
            time = TM.GetElapsedTime() ;
            TM.Reset() ;
            // printf("Elapsed time %f\n", time) ;
            break ;
        case '0':
            //reset your object
            break ;
        case 'm':
            if( Recording == 1 )
            {
                printf("Frame recording disabled.\n") ;
                Recording = 0 ;
            }
            else
            {
                printf("Frame recording enabled.\n") ;
                Recording = 1  ;
            }
            FrSaver.Toggle(Width);
            break ;
        case 'h':
        case '?':
            instructions();
            break;
    }
    glutPostRedisplay() ;

}


/**********************************************
 PROC: myMouseCB()
 DOES: handles the mouse button interaction
 
 -- don't change
 **********************************************************/
void myMouseCB(int button, int state, int x, int y)
{
    Button = button ;
    if( Button == GLUT_LEFT_BUTTON && state == GLUT_DOWN )
    {
        HVect arcball_coords;
        arcball_coords.x = 2.0*(float)x/(float)Width-1.0;
        arcball_coords.y = -2.0*(float)y/(float)Height+1.0;
        Ball_Mouse(Arcball, arcball_coords) ;
        Ball_Update(Arcball);
        Ball_BeginDrag(Arcball);

    }
    if( Button == GLUT_LEFT_BUTTON && state == GLUT_UP )
    {
        Ball_EndDrag(Arcball);
        Button = -1 ;
    }
    if( Button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN )
    {
        PrevY = y ;
    }


    // Tell the system to redraw the window
    glutPostRedisplay() ;
}


/**********************************************
 PROC: myMotionCB()
 DOES: handles the mouse motion interaction
 
 -- don't change
 **********************************************************/
void myMotionCB(int x, int y)
{
    if( Button == GLUT_LEFT_BUTTON )
    {
        HVect arcball_coords;
        arcball_coords.x = 2.0*(float)x/(float)Width - 1.0 ;
        arcball_coords.y = -2.0*(float)y/(float)Height + 1.0 ;
        Ball_Mouse(Arcball,arcball_coords);
        Ball_Update(Arcball);
        glutPostRedisplay() ;
    }
    else if( Button == GLUT_RIGHT_BUTTON )
    {
        if( y - PrevY > 0 )
            Zoom  = Zoom * 1.03 ;
        else 
            Zoom  = Zoom * 0.97 ;
        PrevY = y ;
        glutPostRedisplay() ;
    }
}


/**********************************************
 PROC: myReshape()
 DOES: handles the window being resized
 
 -- don't change
 **********************************************************/
void myReshape(int w, int h)
{
    Width = w;
    Height = h;
    
    glViewport(0, 0, w, h);
    
    mat4 projection = Perspective(50.0f, (float)w/(float)h, 1.0f, 1000.0f);
    glUniformMatrix4fv( uProjection, 1, GL_TRUE, projection );
}


/*********************************************************
 **********************************************************
 **********************************************************
 
 PROC: display()
 DOES: this gets called by the event handler to draw the scene
       so this is where you need to build your BEE
 
 MAKE YOUR CHANGES AND ADDITIONS HERE
 
 ** Add other procedures, such as drawLegs
 *** Use a hierarchical approach
 
 **********************************************************
 **********************************************************
 **********************************************************/
void display(void)
{
    // Clear the screen with the background colour (set in myinit)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    mat4 model_trans(1.0f);
    mat4 view_trans(1.0f);
    
    view_trans *= Translate(0.0f, 0.0f, -15.0f); //the same effect as zoom out
    
    // below deals with zoom in/out by mouse
    HMatrix r;
    Ball_Value(Arcball,r);
    mat4 mat_arcball_rot(
                         r[0][0], r[0][1], r[0][2], r[0][3],
                         r[1][0], r[1][1], r[1][2], r[1][3],
                         r[2][0], r[2][1], r[2][2], r[2][3],
                         r[3][0], r[3][1], r[3][2], r[3][3]);
    view_trans *= mat_arcball_rot;
    view_trans *= Scale(Zoom);
    
    glUniformMatrix4fv( uView, 1, GL_TRUE, model_view );
    
    mvstack.push(model_trans);//push, now identity is on the stack
    
   
/**************************************************************
   Your drawing/modeling starts here
***************************************************************/

    view_trans *= LookAt(eye, ref, up); // change camera viewing
    
    // model sky block side (back)
    model_trans *= Translate(0, 0, -25); // change z to -25
    model_trans *= Scale(1000, 1000, 1);
    model_view = view_trans * model_trans;
    set_colour(0.19, 0.6, 0.8);
    drawCube();
    
    model_trans = mvstack.pop();
    mvstack.push(model_trans);
    
    // model sky block top
    model_trans *= Translate(0, 35, -25); // change z to -25
    model_trans *= Scale(1000, 1, 1000);
    model_view = view_trans * model_trans;
    set_colour(0.19, 0.6, 0.8);
    drawCube();
    
    model_trans = mvstack.pop();
    mvstack.push(model_trans);
    
    // model sky side (right)
    model_trans *= Translate(25, 0, -25);
    model_trans *= Scale(1, 1000, 1000);
    model_view = view_trans * model_trans;
    set_colour(0.19, 0.6, 0.8);
    drawCube();
    
    model_trans = mvstack.pop();
    mvstack.push(model_trans);
    
    // model sky side (left)
    model_trans *= Translate(-31.5, 0, -25);
    model_trans *= Scale(1, 1000, 1000);
    model_view = view_trans * model_trans;
    set_colour(0.19, 0.6, 0.8);
    drawCube();
    
    model_trans = mvstack.pop();
    mvstack.push(model_trans);
    
    // model cliff
    model_trans *= Translate(-21, -7, -3.5);
    mvstack.push(model_trans);
    model_trans *= Scale(20, 60, 5);
    model_view = view_trans * model_trans;
    set_colour(0.65, 0.5, 0.39);
    drawCube();
    
    model_trans = mvstack.pop();
    mvstack.push(model_trans);
    // model blood 1
    if (impact == true) {
        model_trans *= Translate(17, -28, 0);
        model_trans *= Scale(0.75, 0.25, 0.75);
        model_trans *= Scale(spreadx1, 1, spreadz1);
        model_view = view_trans * model_trans;
        set_colour(1, 0, 0);
        drawSphere();
    }
    
    model_trans = mvstack.pop();
    mvstack.push(model_trans);
    // model blood 2
    
    if (impact == true) {
        model_trans *= Translate(15, -28, 1);
        model_trans *= Scale(0.75, 0.25, 0.75);
        model_trans *= Scale(spreadx2, 1, spreadz2);
        model_view = view_trans * model_trans;
        set_colour(1, 0, 0);
        drawSphere();
    }
    
    model_trans = mvstack.pop();
    mvstack.push(model_trans);
    // model blood 3
    
    if (impact == true) {
        model_trans *= Translate(14, -28, -1);
        model_trans *= Scale(0.75, 0.25, 0.75);
        model_trans *= Scale(spreadx2, 1, spreadz2);
        model_view = view_trans * model_trans;
        set_colour(1, 0, 0);
        drawSphere();
    }
    
    mvstack.pop(); // pop cliff's transformation
    
    model_trans = mvstack.pop();
    mvstack.push(model_trans);
    
    // model ground
    model_trans *= Translate(-10, -60, -3.5);
    model_trans *= Scale(200, 50, 200);
    model_view = view_trans * model_trans;
    set_colour(0.33, 0.33, 0.33);
    drawCube();

    model_trans = mvstack.pop();
    mvstack.push(model_trans);
    
    // model body
    model_trans = mvstack.pop();
    
    model_trans *= Translate(tx, ty, tz);
    model_trans *= RotateY(90);
    model_trans *= RotateX(facedownAngle);
    model_trans *= Scale(1.5, 3.6, 1.5);
    model_view = view_trans * model_trans;
    set_colour(0.0f, 0.0f, 0.8f); // blue
    drawCube();
    
    mvstack.push(model_trans);
    
    model_trans = mvstack.pop();
    mvstack.push(model_trans); // holds body's transformation for arm
    
    // model right arm
    model_trans *= RotateZ(45+rotatearmAngle);
    model_trans *= Translate(0.5, -0.4, 0);
    mvstack.push(model_trans); // hold arm's transformation for hand
    model_trans *= Scale(0.25, 0.6, 0.5);
    model_view = view_trans * model_trans;
    set_colour(0.8, 0.0, 0.0); // red
    drawCube();
    
    model_trans = mvstack.pop();
    
    // model right hand
    model_trans *= Translate(0, -0.4, 0);
    model_trans *= Scale(0.2, 0.2, 0.2);
    model_view = view_trans * model_trans;
    set_colour(0.86, 0.57, 0.43);
    drawSphere();
    
    model_trans = mvstack.pop();
    mvstack.push(model_trans);
    
    // model left arm
    model_trans *= RotateZ(-(45+rotatearmAngle));
    model_trans *= Translate(-0.5, -0.4, 0);
    mvstack.push(model_trans); // hold arm's transformation for hand
    model_trans *= Scale(0.25, 0.6, 0.5);
    model_view = view_trans * model_trans;
    set_colour(0.8, 0.0, 0.0); // red
    drawCube();
    
    model_trans = mvstack.pop();
    
    // model left hand
    model_trans *= Translate(0, -0.4, 0);
    model_trans *= Scale(0.2, 0.2, 0.2);
    model_view = view_trans * model_trans;
    set_colour(0.86, 0.57, 0.43);
    drawSphere();
    
    model_trans = mvstack.pop();
    mvstack.push(model_trans);
    
    // model head
    model_trans *= Translate(0, 0.75, 0.1);
    model_trans *= Scale(0.75, 0.33, 0.75); // brown
    
    model_trans *= RotateX(m_angle);
    mvstack.push(model_trans);
    
    model_view = view_trans * model_trans;
    set_colour(0.86, 0.57, 0.43);
    drawSphere();
    
    model_trans = mvstack.pop();
    mvstack.push(model_trans);
    
    // model eye 1
    model_trans *= Translate(0.25, 0.45, 0.95);
    model_trans *= Scale(0.26, 0.15, 0);
    
    model_view = view_trans * model_trans;
    set_colour(0, 0, 0);
    drawCube();
    
    model_trans = mvstack.pop();
    mvstack.push(model_trans);
    
    // model eye 2
    model_trans *= Translate(-0.25, 0.45, 0.95);
    model_trans *= Scale(0.26, 0.15, 0);

    model_view = view_trans * model_trans;
    set_colour(0, 0, 0);
    drawCube();
    
    model_trans = mvstack.pop();
    mvstack.push(model_trans);
    
    // model mouth
    model_trans *= Translate(0, 0, 1);
    model_trans *= Scale(0.7, 0.175, 0);
    
    model_view = view_trans * model_trans;
    set_colour(0, 0, 0);
    drawCube();
    
    mvstack.pop();
    model_trans = mvstack.pop();
    mvstack.push(model_trans);
    
    // model leg 1
    model_trans *= Translate(-0.25, -0.7, 0);
    model_trans *= Scale(0.3, 0.5, 0.5);
    model_view = view_trans * model_trans;
    set_colour(0.75, 0.85, 0.85); // light blue
    drawCube();
    
    model_trans = mvstack.pop();
    mvstack.push(model_trans);
    
    // model leg 2
    model_trans *= Translate(0.25, -0.7, 0);
    model_trans *= Scale(0.3, 0.5, 0.5);
    model_view = view_trans * model_trans;
    set_colour(0.75, 0.85, 0.85); // light blue
    drawCube();
    
    mvstack.pop();
    
    
/**************************************************************
     Your drawing/modeling ends here
 ***************************************************************/
    
    glutSwapBuffers();
    if(Recording == 1)
        FrSaver.DumpPPM(Width, Height);
}


/*********************************************************
 **********************************************************
 **********************************************************
 
 PROC: idle()
 DOES: this gets called when nothing happens. 
       That's not true. 
       A lot of things happen can here.
       This is where you do your animation.
 
 MAKE YOUR CHANGES AND ADDITIONS HERE
 
 **********************************************************
 **********************************************************
 **********************************************************/
void idle(void)
{
    if( Animate == 1 )
    {
        // TM.Reset() ; // commenting out this will make the time run from 0
        // leaving 'Time' counts the time interval between successive calls to idleCB
        if( Recording == 0 )
            TIME = TM.GetElapsedTime() ;
        else
            TIME += 0.033 ; // save at 30 frames per second.
        
        //Your code starts here
        
        if (TIME < 4.0) {
            tx = -23 + 4*TIME;
        }
        else if (TIME >= 4.0 && TIME < 6) { // stands at edge of cliff
            m_angle = 7 * TIME;
            rotatearmAngle = 5*sin(15*TIME);
            eye.x = tx + 2;
            eye.y = ty;
            eye.z = tz;
            ref.x = tx;
            ref.y = ty;
            ref.z = tz;
        }
        else if (TIME > 6 && TIME < 13) {
            rotatearmAngle = 5*sin(15*TIME);
            eye.x = 10*cos(TIME);
            eye.y = ty;
            eye.z = 10*sin(TIME);
            ref.x = tx;
            ref.y = ty;
            ref.z = tz;
        }
        else if (TIME >= 13 && TIME < 13.1) {
            facedownAngle = 90;
            m_angle = -45;
        }
        else if (TIME > 13.1 && TIME < 15) {
            ty = 26 + (13.1*8) - 8*TIME;
            eye.x = -6; // starts at -7.05858
            eye.y = 15; // starts at 26
            eye.z = 0; // starts at 0
            ref.x = tx;
            ref.y = ty;
            ref.z = tz;
        }
        else if (TIME > 15 && TIME < 17.6) {
            ty = 10.0976 + (15*8) - 8*TIME;
            eye.x = 5; // starts at 10.0976
            eye.y = 15;
            eye.z = 0;
            
            ref.x = -3;
            ref.y = 12.05;
            ref.z = 0;
        }
        else if (TIME > 17.6 && TIME < 20.6) {
            ty = -10.6637 + (17.6*8) - 8*TIME; // starts at -10.66
            eye.x = 5;
            eye.y = -30;
            eye.z = 0;
            ref.x = -3;
            ref.y = -30;
            ref.z = 0;
        }
        else if (TIME > 20.6 && TIME < 23) {
            impact = true;
            spreadx1 = TIME - 18.6;
            spreadz1 = TIME - 18.6;
            spreadx2 = TIME - 19.58;
            spreadz2 = TIME - 19.58;
        }
        
        //Your code ends here
        
        printf("TIME %f\n", TIME) ;
        glutPostRedisplay() ;
    }
}

/*********************************************************
     PROC: main()
     DOES: calls initialization, then hands over control
           to the event handler, which calls 
           display() whenever the screen needs to be redrawn
**********************************************************/

int main(int argc, char** argv) 
{
    glutInit(&argc, argv);
    // If your code fails to run, uncommenting these lines may help.
    //glutInitContextVersion(3, 2);
    //glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowPosition (0, 0);
    glutInitWindowSize(Width,Height);
    glutCreateWindow(argv[0]);
    printf("GL version %s\n", glGetString(GL_VERSION));
    glewExperimental = GL_TRUE;
    glewInit();
    
    instructions();
    myinit(); //performs most of the OpenGL intialization
    
    
    glutKeyboardFunc( myKey );   //keyboard event handler
    glutMouseFunc(myMouseCB) ;   //mouse button event handler
    glutMotionFunc(myMotionCB) ; //mouse motion event handler
    
    glutReshapeFunc (myReshape); //reshape event handler
    glutDisplayFunc(display);    //draw a new scene
    glutIdleFunc(idle) ;         //when nothing happens, do animaiton

    
    glutMainLoop();

    TM.Reset() ;
    return 0;         // never reached
}




