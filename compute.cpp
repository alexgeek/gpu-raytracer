#include "compute.h"

void cl_info() {
    int i, j;
    char* info;
    size_t infoSize;
    cl_uint platformCount;
    cl_platform_id *platforms;
    const char* attributeNames[5] = { "Name", "Vendor",
            "Version", "Profile", "Extensions" };
    const cl_platform_info attributeTypes[5] = { CL_PLATFORM_NAME, CL_PLATFORM_VENDOR,
            CL_PLATFORM_VERSION, CL_PLATFORM_PROFILE, CL_PLATFORM_EXTENSIONS };
    const int attributeCount = sizeof(attributeNames) / sizeof(char*);

    // get platform count
    clGetPlatformIDs(5, NULL, &platformCount);

    // get all platforms
    platforms = (cl_platform_id*) malloc(sizeof(cl_platform_id) * platformCount);
    clGetPlatformIDs(platformCount, platforms, NULL);

    // for each platform print all attributes
    for (i = 0; i < platformCount; i++) {

        printf("%d. Platform \n", i+1);

        for (j = 0; j < attributeCount; j++) {

            // get platform attribute value size
            clGetPlatformInfo(platforms[i], attributeTypes[j], 0, NULL, &infoSize);
            info = (char*) malloc(infoSize);

            // get platform attribute value
            clGetPlatformInfo(platforms[i], attributeTypes[j], infoSize, info, NULL);

            printf("  %d.%d %-11s: %s\n", i+1, j+1, attributeNames[j], info);
            free(info);

        }

        printf("\n");

    }

    free(platforms);
}

/**
* Selects CL platform/device capable of CL/GL interop.
*/
void cl_select(cl_platform_id* platform_id, cl_device_id* device_id) {
    cl_int err;
    int i;
    char* info;
    size_t infoSize;
    cl_uint platformCount;
    cl_platform_id *platforms;

    // get platform count
    err = clGetPlatformIDs(5, NULL, &platformCount);
    CHECK_ERR(err);

    // get all platforms
    platforms = (cl_platform_id*) malloc(sizeof(cl_platform_id) * platformCount);
    err = clGetPlatformIDs(platformCount, platforms, NULL);
    CHECK_ERR(err);

    // for each platform print all attributes
    for (i = 0; i < platformCount; i++) {

        printf("%d. Checking Platform \n", i+1);

        // get platform attribute value size
        err = clGetPlatformInfo(platforms[i], CL_PLATFORM_EXTENSIONS, 0, NULL, &infoSize);
        CHECK_ERR(err);
        info = (char*) malloc(infoSize);

        // get platform attribute value
        err = clGetPlatformInfo(platforms[i], CL_PLATFORM_EXTENSIONS, infoSize, info, NULL);
        CHECK_ERR(err);

        if(strstr(info, GL_SHARING_EXTENSION) != NULL) {
            cl_uint num_devices;
            cl_device_id* devices;

            // Get the number of GPU devices available to the platform
            err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, 0, NULL, &num_devices);
            CHECK_ERR(err);

            // Create the device list
            devices = new cl_device_id [num_devices];
            err  = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, num_devices, devices, NULL);
            CHECK_ERR(err);

            int d;
            for(d = 0; d < num_devices; d++) {

                // get device attribute value size
                size_t extensionSize;
                err = clGetDeviceInfo(devices[d], CL_DEVICE_EXTENSIONS, 0, NULL, &extensionSize );
                CHECK_ERR(err);

                if(extensionSize > 0) {
                    char* extensions = (char*)malloc(extensionSize);
                    err = clGetDeviceInfo(devices[d], CL_DEVICE_EXTENSIONS, extensionSize, extensions, &extensionSize);
                    CHECK_ERR(err);

                    if(strstr(info, GL_SHARING_EXTENSION) != NULL) {
                        printf("Found Compatible platform %d and device %d out of %d .\n", i, d, num_devices);
                        *platform_id = platforms[i];
                        *device_id = devices[d];

                        // TODO remove
                        // break;
                    }

                    free(extensions);
                }

            }
        }

        free(info);
        printf("\n");

    }

    free(platforms);
}

void cl_select_context(cl_platform_id* platform, cl_device_id* device, cl_context* context) {
    cl_int err;
#if defined (__APPLE__)
    CGLContextObj kCGLContext = CGLGetCurrentContext();
    CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext);
    cl_context_properties props[] =
    {
        CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE, (cl_context_properties)kCGLShareGroup,
        0
    };
    *context = clCreateContext(props, 0,0, NULL, NULL, &err);
#else
    #ifdef UNIX
    cl_context_properties props[] =
    {
        CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(),
        CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(),
        CL_CONTEXT_PLATFORM, (cl_context_properties)platform,
        0
    };
    *context = clCreateContext(props, 1, &cdDevices[uiDeviceUsed], NULL, NULL, &err);
    #else // Win32
    cl_context_properties props[] =
    {
        CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
        CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
        CL_CONTEXT_PLATFORM, (cl_context_properties)*platform,
        0
    };
    *context = clCreateContext(props, 1, device, NULL, NULL, &err);
    CHECK_ERR(err);
#endif
#endif
}

void cl_load_kernel(cl_context* context, cl_device_id* device, const char* source, cl_command_queue* command_queue, cl_kernel* kernel) {
    cl_mem memobj;
    cl_int err;
    cl_program program;
    char string[MEM_SIZE];

    FILE *fp;
    char *source_str;
    size_t source_size;

    /* Load the source code containing the kernel*/
    fp = fopen(source, "r");
    if (!fp) {
        fprintf(stderr, "Failed to load kernel.\n");
        exit(1);
    }
    source_str = (char *) malloc(MAX_SOURCE_SIZE);
    source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
    fclose(fp);

    // create a command queue
    *command_queue = clCreateCommandQueue(*context, *device, 0, &err);
    CHECK_ERR(err);


    memobj = clCreateBuffer(*context, CL_MEM_READ_WRITE, MEM_SIZE * sizeof(char), NULL, &err);
    CHECK_ERR(err);

    /* Create Kernel Program from the sour
    ce */
    program = clCreateProgramWithSource(*context, 1, (const char **) &source_str,
            (const size_t *) &source_size, &err);
    CHECK_ERR(err);


    /* Build Kernel Program */
    err = clBuildProgram(program, 1, device, NULL, NULL, NULL);
    if(err != CL_SUCCESS) {
        size_t len;
        cl_build_status build_status;
        char buffer[204800];
        err = clGetProgramBuildInfo(program, *device, CL_PROGRAM_BUILD_STATUS, sizeof(build_status), (void *)&build_status, &len);
        CHECK_ERR(err);
        err = clGetProgramBuildInfo(program, *device, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        CHECK_ERR(err);
        printf("Build Log:\n%s\n", buffer);
        exit(1);
    }

    /* Create OpenCL Kernel */
    *kernel = clCreateKernel(program, "sine_wave", &err);
    CHECK_ERR(err);
}

void cl_set_constant_args(cl_kernel * kernel, cl_mem* cl_texture, unsigned int width, unsigned int height) {
    cl_int err;
    err = clSetKernelArg(*kernel, 0, sizeof(cl_mem), (void*)cl_texture);
    CHECK_ERR(err);
    err = clSetKernelArg(*kernel, 1,  sizeof(unsigned int), &width);
    CHECK_ERR(err);
    err = clSetKernelArg(*kernel, 2,  sizeof(unsigned int), &height);
    CHECK_ERR(err);
}

void cl_create_texture(cl_context *context, GLuint *texture, cl_mem *cl_texture, unsigned int width, unsigned int height) {
    cl_int err;

    CHECK_GL(glGenTextures(1, texture));

    CHECK_GL(glBindTexture(GL_TEXTURE_2D, *texture));

    CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    //need to set GL_NEAREST (not GL_NEAREST_MIPMAP_* which would cause CL_INVALID_GL_OBJECT later)
    CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    CHECK_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    //specify texture dimensions, format etc
    CHECK_GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0));

    // create a CL buffer from the vbo
    *cl_texture = clCreateFromGLTexture2D(*context, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, *texture, &err);
    CHECK_ERR(err);
}

float anim = 0;
void cl_run_kernel(cl_command_queue* command_queue, cl_kernel* kernel, cl_mem*texture_cl, unsigned int width, unsigned int height) {
    cl_int err;
    // map OpenGL buffer object for writing from OpenCL
    //glFinish();
    err = clEnqueueAcquireGLObjects(*command_queue, 1, texture_cl, 0,0,0);
    CHECK_ERR(err);

    // Set arg 3 and execute the kernel
    size_t work[] = {width, height};
    anim = (anim + 0.01);
    err = clSetKernelArg(*kernel, 3, sizeof(float), &anim);
    CHECK_ERR(err);

    err = clEnqueueNDRangeKernel(*command_queue, *kernel, 2, NULL, work, NULL, 0,0,0 );
    CHECK_ERR(err);

    err = clEnqueueReleaseGLObjects(*command_queue, 1, texture_cl, 0,0,0);
    CHECK_ERR(err);

    err = clFinish(*command_queue);
    CHECK_ERR(err);
}