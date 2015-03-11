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
    int i;
    char* info;
    size_t infoSize;
    cl_uint platformCount;
    cl_platform_id *platforms;

    // get platform count
    clGetPlatformIDs(5, NULL, &platformCount);

    // get all platforms
    platforms = (cl_platform_id*) malloc(sizeof(cl_platform_id) * platformCount);
    clGetPlatformIDs(platformCount, platforms, NULL);

    // for each platform print all attributes
    for (i = 0; i < platformCount; i++) {

        printf("%d. Checking Platform \n", i+1);

        // get platform attribute value size
        clGetPlatformInfo(platforms[i], CL_PLATFORM_EXTENSIONS, 0, NULL, &infoSize);
        info = (char*) malloc(infoSize);

        // get platform attribute value
        clGetPlatformInfo(platforms[i], CL_PLATFORM_EXTENSIONS, infoSize, info, NULL);

        if(strstr(info, GL_SHARING_EXTENSION) != NULL) {
            cl_uint num_devices;
            cl_device_id* devices;

            // Get the number of GPU devices available to the platform
            clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, 0, NULL, &num_devices);

            // Create the device list
            devices = new cl_device_id [num_devices];
            clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, num_devices, devices, NULL);

            int d;
            for(d = 0; d < num_devices; d++) {

                // get device attribute value size
                size_t extensionSize;
                clGetDeviceInfo(devices[d], CL_DEVICE_EXTENSIONS, 0, NULL, &extensionSize );

                if(extensionSize > 0) {
                    char* extensions = (char*)malloc(extensionSize);
                    clGetDeviceInfo(devices[d], CL_DEVICE_EXTENSIONS, extensionSize, extensions, &extensionSize);

                    if(strstr(info, GL_SHARING_EXTENSION) != NULL) {
                        printf("Found Compatible platform.\n");
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