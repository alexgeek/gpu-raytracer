#ifndef COMPUTE_H
#define COMPUTE_H

#if defined (__APPLE__) || defined(MACOSX)
    #define GL_SHARING_EXTENSION "cl_APPLE_gl_sharing"
#else
    #define GL_SHARING_EXTENSION "cl_khr_gl_sharing"
#endif

#define MEM_SIZE (128)
#define MAX_SOURCE_SIZE (0x100000)

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined _WIN32 || defined __WIN32__ || defined __WINDOWS__ || defined _WIN64
#include <windows.h>
#endif

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include <CL/cl_gl.h>

#define CHECK_ERR(E) if(E != CL_SUCCESS) fprintf (stderr, "CL ERROR %d in %s:%d\n", E,__FILE__, __LINE__);

void cl_info();
void cl_select(cl_platform_id* platform_id, cl_device_id* device_id);
void cl_select_context(cl_platform_id* platform, cl_device_id* device, cl_context* context);
void cl_load_kernel(cl_context* context, cl_device_id* device, const char* source, cl_command_queue* command_queue, cl_kernel* kernel);
void cl_set_constant_args(cl_kernel * kernel, cl_mem* vbo_cl, unsigned int width, unsigned int height);
void cl_vbo(cl_context * context, GLuint* vbo, cl_mem* vbo_cl, unsigned int width, unsigned int height);
void cl_run_kernel(cl_command_queue* command_queue, cl_kernel* kernel, cl_mem* vbo_cl, unsigned int width, unsigned int height);
#ifdef __cplusplus
}
#endif

#endif