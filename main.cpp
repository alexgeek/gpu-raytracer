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

#define FPS_ENABLED 1

#include "compute.h"

using namespace glm;

double time = 0;
#ifdef FPS_ENABLED
double fps_update_time = 0;
unsigned int frames = 0;
#endif

GLuint texture;

unsigned int width = 800;
unsigned int height = 600;
unsigned int window_width = width;
unsigned int window_height = height;


// CL
cl_mem texture_cl;
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
  const double current_time = glfwGetTime();
  // const double time_delta = current_time - time;

  #ifdef FPS_ENABLED
  frames++;
  if(current_time - fps_update_time >= 1.0) {
    char title[64];
    sprintf(title, "GPU RAY TRACER (%f FPS)", 1000.0f / frames);
    glfwSetWindowTitle(window, title);
    fps_update_time = current_time;
    frames = 0;
  }
  #endif

  /*** run the ray tracing kernel ***/
  cl_run_kernel(&command_queue, &kernel, &texture_cl, width, height);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  CHECK_GL(glBindTexture(GL_TEXTURE_2D, texture));
  glBegin(GL_QUADS);
  glTexCoord2f(0.0f, 0.0f);
  glVertex3f(-1.0f, -1.0f, 0.1f);

  glTexCoord2f(1.0f, 0.0f);
  glVertex3f(1.0f, -1.0f, 0.1f);

  glTexCoord2f(1.0f, 1.0f);
  glVertex3f(1.0f, 1.0f, 0.1f);

  glTexCoord2f(0.0f, 1.0f);
  glVertex3f(-1.0f, 1.0f, 0.1f);
  glEnd();

  time = current_time;
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
  CHECK_GL(glDisable(GL_DEPTH_TEST));
  CHECK_GL(glDisable(GL_LIGHTING));
  CHECK_GL(glEnable(GL_TEXTURE_2D));

  // viewport
  glViewport(0, 0, window_width, window_height);

  // projection
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glPerspective(60.0, window_width/window_height, 0.1, 10.0);

  // set view matrix
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0.0, 0.0, -1.0);
}


int main(void) {
  GLFWwindow *window;

  glfwSetErrorCallback(error_callback);

  if (!glfwInit())
    exit(EXIT_FAILURE);

  /*glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);*/
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
  glfwWindowHint(GLFW_REFRESH_RATE, GLFW_DONT_CARE);


  window = glfwCreateWindow(window_width, window_height, "GPU Ray Tracer", NULL, NULL);
  if (!window) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  const char* name = glfwGetMonitorName(monitor);
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);
  printf("%s (%d,%d,%d) %d Hz\n", name, mode->redBits, mode->greenBits, mode->blueBits, mode->refreshRate);


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
  cl_create_texture(&context, &texture, &texture_cl, width, height);
  cl_set_constant_args(&kernel, &texture_cl, width, height);
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
