#ifndef COMPUTE_H
#define COMPUTE_H

#if defined (__APPLE__) || defined(MACOSX)
    #define GL_SHARING_EXTENSION "cl_APPLE_gl_sharing"
#else
    #define GL_SHARING_EXTENSION "cl_khr_gl_sharing"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

void cl_info();
void cl_select(cl_platform_id* platform_id, cl_device_id* device_id);

#ifdef __cplusplus
}
#endif

#endif