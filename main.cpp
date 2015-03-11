#include <stdio.h>
#include <stdlib.h>

#include <GL/glew.h>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#define MEM_SIZE (128)
#define MAX_SOURCE_SIZE (0x100000)

#include "compute.h"

using namespace glm;

GLuint vbo;
unsigned int width = 256;
unsigned int height = 256;
const unsigned int window_width = 512;
const unsigned int window_height = 512;

// CL
cl_mem vbo_cl;
cl_platform_id pid;
cl_device_id  did;
cl_context context;
cl_kernel kernel;
cl_command_queue command_queue;

static void error_callback(int error, const char *description) {
  fputs(description, stderr);
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GL_TRUE);
}

static void render(GLFWwindow *window) {
  cl_run_kernel(&command_queue, &kernel, &vbo_cl, width, height);

  // check for OpenGL errors
  GLenum err;
  while ((err = glGetError()) != GL_NO_ERROR)
    printf("CLGL error: %d\n", err);

  // clear graphics then render from the vbo
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glVertexPointer(4, GL_FLOAT, 0, 0);
  glEnableClientState(GL_VERTEX_ARRAY);
  glColor3f(1.0, 0.0, 0.0);
  glDrawArrays(GL_POINTS, 0, width * height);
  glDisableClientState(GL_VERTEX_ARRAY);

  // check for OpenGL errors
  while ((err = glGetError()) != GL_NO_ERROR)
    printf("OpenGL error: %d\n", err);
}

void glPerspective(double fov, double aspectRatio, double znear, double zfar)
{
  double ymax, xmax;
  ymax = znear * tanf(fov * M_PI / 360.0);
  xmax = ymax * aspectRatio;
  glFrustum(-xmax, xmax, -ymax, ymax, znear, zfar);
}

void init_gl()
{
  // default initialization
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glDisable(GL_DEPTH_TEST);

  // viewport
  glViewport(0, 0, window_width, window_height);

  // projection
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  //glOrtho(<#(GLdouble)left#>, <#(GLdouble)right#>, <#(GLdouble)bottom#>, <#(GLdouble)top#>, <#(GLdouble)zNear#>, <#(GLdouble)zFar#>)
  //gluPerspective(60.0, (GLfloat)window_width / (GLfloat) window_height, 0.1, 10.0);
  glPerspective(60.0, window_width/window_height, 0.1, 10.0);

  // set view matrix
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0.0, 0.0, -3.0);
}


int main(void) {
  GLFWwindow *window;

  glfwSetErrorCallback(error_callback);

  if (!glfwInit())
    exit(EXIT_FAILURE);

  /*glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);*/

  window = glfwCreateWindow(window_width, window_height, "OpenGL Boilerplate", NULL, NULL);
  if (!window) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwMakeContextCurrent(window);

  glewExperimental = GL_TRUE;
  GLenum err = glewInit();
  if (err != GLEW_OK) {
    fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
  } else {
    // get version info
    const GLubyte *renderer = glGetString(GL_RENDERER); // get renderer string
    const GLubyte *version = glGetString(GL_VERSION); // version as a string
    printf("Renderer: %s\n", renderer);
    printf("OpenGL version supported %s\n", version);
  }

  init_gl();

  // CL
  cl_info();
  cl_select(&pid, &did);
  cl_select_context(&pid, &did, &context);
  cl_load_kernel(&context, &did, "./trace.cl", &command_queue, &kernel);
  cl_vbo(&context, &vbo, &vbo_cl, width, height);
  cl_set_constant_args(&kernel, &vbo_cl, width, height);
  // END CL

  glfwSetKeyCallback(window, key_callback);

  while (!glfwWindowShouldClose(window)) {
    render(window);

    glfwSwapBuffers(window);
    glfwPollEvents();
    //glfwWaitEvents();
  }

  glfwDestroyWindow(window);

  glfwTerminate();
  exit(EXIT_SUCCESS);
}
