//c libs
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

//opengl (only apple includes tested)
#ifdef __APPLE__
    #include <OpenGL/gl3.h>
    #include <OpenGL/gl.h>
    #include <OpenGL/glu.h>
#else
    #include <GL/gl3.h>
    #include <GL/gl.h>
    #include <GL/glu.h>
#endif

//glfw
#define GLFW_INCLUDE_GL3  /* don't drag in legacy GL headers. */
#define GLFW_NO_GLU       /* don't drag in the old GLU lib - unless you must. */
#include <GL/glfw.h>



// setup -----------------------------------------------------------

//vertex shader w/ two rotation matricies stored
char *vertex_shader = "\
#version 330\n\
layout(location = 0) in vec3 position;\
uniform vec3 offset;\
uniform mat4 perspective;\
uniform vec2 angle;\
uniform vec4 color;\
smooth out vec4 theColor;\
void main(){\
  mat4 xRMatrix = mat4(cos(angle.x), 0.0, sin(angle.x), 0.0,\
                        0.0, 1.0, 0.0, 0.0,\
                        -sin(angle.x), 0.0, cos(angle.x), 0.0,\
                        0.0, 0.0, 0.0, 1.0);\
  mat4 yRMatrix = mat4(1.0, 0.0, 0.0, 0.0,\
                  0.0, cos(angle.y), -sin(angle.y), 0.0,\
                  0.0, sin(angle.y), cos(angle.y), 0.0,\
                  0.0, 0.0, 0.0, 1.0);\
  vec4 rotatedPosition = vec4( position.xyz, 1.0f ) * xRMatrix * yRMatrix;\
  vec4 cameraPos = rotatedPosition + vec4(offset.x, offset.y, offset.z, 0.0);\
  gl_Position = perspective * cameraPos;\
  theColor = mix( vec4( color.x, color.y, color.z, color.w ), vec4( 0.0f, color.y, color.z, color.w ), position.y / 10 );\
}";

//fragment shader
char *fragment_shader = "\
#version 330\n\
smooth in vec4 theColor;\
out vec4 outputColor;\
void main(){\
  outputColor = theColor;\
}";


//init gl vars: buffers
GLuint vao;
GLuint buffer_3d_1;
GLuint buffer_3d_2;
GLuint buffer_2d;
GLuint buffer_axis;
GLuint buffer_axis_marks;
//shader program
GLuint program;
//uniform locations
GLuint uniform_offset;
GLuint uniform_angle;
GLuint uniform_color;
GLuint uniform_rotation;
GLuint uniform_perspective;


//number of graph samples to make
int graph_samples = 30;
//size of axis & marks
int graph_size = 100;
//scale y axis (because otherwise its between 0-1 while x is -30 to +30)
int graph_y_scale = 50;

//initial position + angle variables for shader
float xpos = 0;
float ypos = 0;
float zpos = -40;
float xang = 0;
float yang = 0;

//do we draw 2d, 3d?
int draw_2d = 1;
int draw_3d = 1;


//a point in 3d space (used on 2d & 3d graphs)
struct position {
  GLfloat x;
  GLfloat y;
  GLfloat z;
};
//axis
struct position axis[6];
struct position axis_marks[600];

//to stop cc complaining
void GLFWCALL keypress();
void GLFWCALL resize();


// functions -----------------------------------------------------------

//setup everything
void init() {
    //init glfw, open window, set opengl 3.2 + core profile only
    glfwInit();
    glfwOpenWindowHint( GLFW_OPENGL_VERSION_MAJOR, 3 );
    glfwOpenWindowHint( GLFW_OPENGL_VERSION_MINOR, 2 );
    glfwOpenWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
    glfwOpenWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
    glfwOpenWindow( 1000, 800, 8, 8, 8, 8, 0, 0, GLFW_WINDOW );
    glViewport( 0, 0, 1000, 800 );
    glfwSetWindowTitle( "2D & 3D Graphing in OpenGL" );
    //bind keyboard callback
    glfwSetKeyCallback( keypress );
    //bind window resize callback
    glfwSetWindowSizeCallback( resize );

    //compile vertex shader
    GLuint v_shader = glCreateShader( GL_VERTEX_SHADER );
    glShaderSource( v_shader, 1, (const GLchar**)&vertex_shader, 0 );
    glCompileShader( v_shader );
    //compile frag shader
    GLuint f_shader = glCreateShader( GL_FRAGMENT_SHADER );
    glShaderSource( f_shader, 1, (const GLchar**)&fragment_shader, 0 );
    glCompileShader( f_shader );
    //create the shader program
    program = glCreateProgram();
    glAttachShader( program, v_shader );
    glAttachShader( program, f_shader );
    glLinkProgram( program );

    //vertex array object state
    glGenVertexArrays( 1, &vao );
    glBindVertexArray( vao );

    //get uniform locations in shaders
    uniform_offset = glGetUniformLocation( program, "offset" );
    uniform_angle = glGetUniformLocation( program, "angle" );
    uniform_color = glGetUniformLocation( program, "color" );
    uniform_perspective = glGetUniformLocation( program, "perspective" );

    //setup perspective
    float fFrustumScale = 1.0f;
    float fzNear = 1.0f;
    float fzFar = 100.0f;
    //THE MATRIX
    float theMatrix[16];
    memset(theMatrix, 0, sizeof(float) * 16);
    //build perspective matrix
    theMatrix[0] = fFrustumScale;
    theMatrix[5] = fFrustumScale;
    theMatrix[10] = (fzFar + fzNear) / (fzNear - fzFar);
    theMatrix[14] = (2 * fzFar * fzNear) / (fzNear - fzFar);
    theMatrix[11] = -1.0f;
    //bind data to shader
    glUseProgram( program );
    glUniformMatrix4fv( uniform_perspective, 1, GL_TRUE, theMatrix ); //perspective matrix
    glUniform3f( uniform_offset, xpos, ypos, zpos ); //position offset
    glUniform2f( uniform_angle, xang, yang); //rotation angles
    glUniform4f( uniform_color, 1, 1, 1, 1 ); //color
    glUseProgram( 0 );

    //axis lines
    //x
    axis[0].x = -graph_size;
    axis[0].y = 0;
    axis[0].z = 0;
    axis[1].x = graph_size;
    axis[1].y = 0;
    axis[1].z = 0;
    //y
    axis[2].x = 0;
    axis[2].y = -graph_size;
    axis[2].z = 0;
    axis[3].x = 0;
    axis[3].y = graph_size;
    axis[3].z = 0;
    //z
    axis[4].x = 0;
    axis[4].y = 0;
    axis[4].z = graph_size;
    axis[5].x = 0;
    axis[5].y = 0;
    axis[5].z = -graph_size;

    //buffer for axis
    glGenBuffers( 1, &buffer_axis);
    glBindBuffer( GL_ARRAY_BUFFER, buffer_axis );
    //assign axis data
    glBufferData( GL_ARRAY_BUFFER, sizeof( axis ), axis, GL_STATIC_DRAW );

    //axis marks
    int i;
    int index = 0;
    //x
    for( i = -graph_size; i < graph_size; ++i ) {
        if( i % 2 != 0 ) continue; //only every 2 points
        axis_marks[index].x = i;
        axis_marks[index].y = 0;
        axis_marks[index].z = 0;
        index++;
        axis_marks[index].x = i;
        axis_marks[index].y = 1;
        axis_marks[index].z = 0;
        index++;
    }
    //y
    for( i = -graph_size; i < graph_size; ++i ) {
        if( i % 2 != 0 ) continue;
        axis_marks[index].x = 0;
        axis_marks[index].y = i;
        axis_marks[index].z = 0;
        index++;
        axis_marks[index].x = 1;
        axis_marks[index].y = i;
        axis_marks[index].z = 0;
        index++;
    }
    //z
    for( i = -graph_size; i < graph_size; ++i ) {
        if( i % 2 != 0 ) continue;
        axis_marks[index].x = 0;
        axis_marks[index].y = 0;
        axis_marks[index].z = i;
        index++;
        axis_marks[index].x = 0;
        axis_marks[index].y = 1;
        axis_marks[index].z = i;
        index++;
    }

    //buffer for axis marks
    glGenBuffers( 1, &buffer_axis_marks);
    glBindBuffer( GL_ARRAY_BUFFER, buffer_axis_marks );
    //assign axis_marks data
    glBufferData( GL_ARRAY_BUFFER, sizeof( axis_marks ), axis_marks, GL_STATIC_DRAW );
}


//generate graph data function
void generate() {
    //reset data
    int estimatedPolys = pow( graph_samples * 2, 2 );
    struct position graph_3d_1[estimatedPolys];
    struct position graph_3d_2[estimatedPolys];
    //2d graph
    estimatedPolys = graph_samples * 2;
    struct position graph_2d[estimatedPolys];

    //ints for point generators
    int i, j;
    int index = 0;

    //generate graph_3d_1
    for( i = -graph_samples; i < graph_samples; i++ ) {
        for( j = -graph_samples; j < graph_samples; j++ ) {
          graph_3d_1[index].x = i;
          graph_3d_1[index].z = j;
          float d = sqrt( i*i + j*j );
          if( d == 0.0 )
            d = 1;

          graph_3d_1[index].y = ( sin( d ) / d ) * graph_y_scale;

          index++;
        }
    }
    //generate graph_3d_2
    index = 0;
    for( i = -graph_samples; i < graph_samples; i++ ) {
        for( j = -graph_samples; j < graph_samples; j++ ) {
          graph_3d_2[index].x = j;
          graph_3d_2[index].z = i;
          float d = sqrt( i*i + j*j );
          if( d == 0.0 )
            d = 1;

          graph_3d_2[index].y = ( sin( d ) / d ) * graph_y_scale;

          index++;
        }
    }

    //generate graph_2d
    for( i = -graph_samples; i < graph_samples; ++i ) {
      index = i + graph_samples;
      //2d - z always 1
      graph_2d[index].z = 0;
      //x always same
      graph_2d[index].x = i;
      //cant divide by 0, set to our top point
      if( i == 0 ) {
        graph_2d[index].y = graph_y_scale;
      } else {
        graph_2d[index].y = sin( i ) / i * graph_y_scale;
      }
    }


    //buffers for 3d graph
    glGenBuffers( 1, &buffer_3d_1 );
    glBindBuffer( GL_ARRAY_BUFFER, buffer_3d_1 );
    glBufferData( GL_ARRAY_BUFFER, sizeof( graph_3d_1 ), graph_3d_1, GL_STATIC_DRAW);
    glGenBuffers( 1, &buffer_3d_2 );
    glBindBuffer( GL_ARRAY_BUFFER, buffer_3d_2 );
    glBufferData( GL_ARRAY_BUFFER, sizeof( graph_3d_2 ), graph_3d_2, GL_STATIC_DRAW);

    //buffer for 2d graph
    glGenBuffers( 1, &buffer_2d );
    glBindBuffer( GL_ARRAY_BUFFER, buffer_2d );
    glBufferData( GL_ARRAY_BUFFER, sizeof( graph_2d ), graph_2d, GL_STATIC_DRAW);

    //console debug
    printf( "3D graph generated, samples used: %d\n", graph_samples * 2 );
    printf( "2D graph generated, samples used: %d\n", graph_samples * 2 );
    int polyCount = ( pow( graph_samples * 2, 2 ) + graph_samples * 2 );
    printf( "Total number of polygons: %d\n", polyCount );
}


//draw function
void draw() {
    int i;

    //use our shader program
    glUseProgram( program );

    //setup offset + angle for everything
    glUniform3f( uniform_offset, xpos, ypos, zpos );
    glUniform2f( uniform_angle, xang, yang );

    //color for axis (green)
    glUniform4f( uniform_color, 0, 1, 0, 1 );
    //draw axis
    glBindBuffer( GL_ARRAY_BUFFER, buffer_axis );
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, 0 );
    glDrawArrays( GL_LINES, 0, 9 );
    glDisableVertexAttribArray( 0 );

    //axis marks
    glBindBuffer( GL_ARRAY_BUFFER, buffer_axis_marks );
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, 0 );
    glDrawArrays( GL_LINES, 0, 600 );
    glDisableVertexAttribArray( 0 );

    //allowed to draw?
    if( draw_3d ) {
        //color for 3d graph pt 1
        glUniform4f( uniform_color, 1, 0.5, 0.1, 1 );
        // graph
        glBindBuffer( GL_ARRAY_BUFFER, buffer_3d_1 );
        glEnableVertexAttribArray( 0 );
        glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, 0 );
        for( i = 0; i < graph_samples * 2; i++ ) {
            glDrawArrays( GL_LINE_STRIP, i * graph_samples * 2, graph_samples * 2 );
        }
        glDisableVertexAttribArray( 0 );
        //color for 3d graph pt 2
        glUniform4f( uniform_color, 1, 0, 0, 1 );
        glBindBuffer(GL_ARRAY_BUFFER, buffer_3d_2 );
        glEnableVertexAttribArray( 0 );
        glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, 0 );
        for( i = 0; i < graph_samples * 2; i++ ) {
            glDrawArrays( GL_LINE_STRIP, i * graph_samples * 2, graph_samples * 2 );
        }
        glDisableVertexAttribArray( 0 );
    }

    //allowed to draw?
    if( draw_2d ) {
        //color for 2d graph
        glUniform4f( uniform_color, 1, 0, 1, 1 );
        glBindBuffer( GL_ARRAY_BUFFER, buffer_2d );
        glEnableVertexAttribArray(0);
        glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, 0 );
        glDrawArrays( GL_LINE_STRIP, 0, graph_samples * 4 );
        glDisableVertexAttribArray( 0 );
    }

    //stop using latest buffer + shader
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glUseProgram(0);
}


//keyboard handling (used for continuous movement of angles/position)
void keyboard() {
    //keyboard management
    if( glfwGetKey('A') == GLFW_PRESS ) {
        xpos = xpos - 0.1;
    }
    if( glfwGetKey('D') == GLFW_PRESS ) {
        xpos = xpos + 0.1;
    }
    if( glfwGetKey('W') == GLFW_PRESS ) {
        ypos = ypos + 0.1;
    }
    if( glfwGetKey('S') == GLFW_PRESS ) {
        ypos = ypos - 0.1;
    }
    if( glfwGetKey('E') == GLFW_PRESS ) {
        zpos = zpos - 0.1;
    }
    if( glfwGetKey('Q') == GLFW_PRESS ) {
        zpos = zpos + 0.1;
    }
    if( glfwGetKey( GLFW_KEY_LEFT ) == GLFW_PRESS ) {
        xang = xang - 0.025;
    }
    if( glfwGetKey( GLFW_KEY_RIGHT ) == GLFW_PRESS ) {
        xang = xang + 0.025;
    }
    if( glfwGetKey( GLFW_KEY_UP ) == GLFW_PRESS ) {
        yang = yang - 0.025;
    }
    if( glfwGetKey( GLFW_KEY_DOWN ) == GLFW_PRESS ) {
        yang = yang + 0.025;
    }
}


//glfw keyboard callback (used for one-press actions)
void GLFWCALL keypress( int key, int action ) {
    if( action == GLFW_PRESS ) {
        //decrease samples
        if( key == '-' ) {
            graph_samples -= 2;
            if( graph_samples == 0 ) graph_samples = 1; //limit to minimum 1
            generate();
        //increase samples (keep going to crash)
        } else if( key == '=' ) {
            graph_samples += 2;
            generate();
        //reset everything
        } else if( key == 'R' ) {
            xang = 0;
            yang = 0;
            zpos = -40;
            xpos = 0;
            ypos = 0;
            draw_2d = 1;
            draw_3d = 1;
            graph_samples = 30;
            generate();
        //2d only
        } else if( key == '2' ) {
            //set perfect position
            xang = 0;
            yang = 0;
            zpos = -20;
            xpos = 0;
            ypos = -20;
            draw_2d = 1;
            draw_3d = 0;
        //3d only
        } else if( key == '3' ) {
            draw_3d = 1;
            draw_2d = 0;
        //both 2d + 3d
        } else if( key == '1' ) {
            draw_3d = 1;
            draw_2d = 1;
        //neither, axis only
        } else if( key == '0' ) {
            draw_3d = 0;
            draw_2d = 0;
        }
    }
}

//glfw resize
void GLFWCALL resize( int width, int height ) {
    glViewport( 0, 0, width, height );
}




// the actual program -----------------------------------------------------------

//main
int main (int argc, char const *argv[])
{
    //init glfw, shaders, axis buffer, etc
    init();

    //generate + buffer graph data
    generate();

    //main loop
    //while window open + not escape key
    while( !glfwGetKey( GLFW_KEY_ESC ) && glfwGetWindowParam( GLFW_OPENED ) ) {
        //clear
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor( 0.1, 0.1, 0.1, 1.0 );
        //draw
        draw();
        //double buffered glfw swap
        glfwSwapBuffers();
        //keyboard
        keyboard();
    } //end of loop

    //end
    glfwTerminate();

    //so long, and thanks for all the fish
    exit( EXIT_SUCCESS );
}