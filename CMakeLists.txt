cmake_minimum_required(VERSION 2.8)
project(tracer)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(OpenCL_INCPATH "C:/Program Files (x86)/AMD APP SDK/2.9-1/include")
set(OpenCL_LIBPATH "C:/Program Files (x86)/AMD APP SDK/2.9-1/lib")

find_package(OpenCL)
message(WARNING "OpenCL found: ${OPENCL_FOUND}")
message(WARNING "OpenCL includes: ${OPENCL_INCLUDE_DIRS}")
message(WARNING "OpenCL CXX includes: ${OPENCL_HAS_CPP_BINDINGS}")
message(WARNING "OpenCL libraries: ${OPENCL_LIBRARIES}")

set(GLFW_SOURCE_DIR "external/glfw")
set(GLM_SOURCE_DIR "external/glm")
set(GLEW_SOURCE_DIR "external/glew")

# Building only the GLFW lib
set(BUILD_SHARED_LIBS OFF CACHE BOOL "")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
set(GLFW_INSTALL OFF CACHE BOOL "")

add_subdirectory(${GLFW_SOURCE_DIR} ${GLEW_SOURCE_DIR})

message(ERROR_FATAL ${CMAKE_SOURCE_DIR})

include_directories(#${OPENCL_INCLUDE_DIRS}
                    ${CMAKE_SOURCE_DIR}/include
                    ${GLFW_SOURCE_DIR}/include
                    ${GLFW_SOURCE_DIR}/deps
                    ${GLM_SOURCE_DIR}
                    ${GLEW_SOURCE_DIR}/include)

add_library(glew STATIC
  ${GLEW_SOURCE_DIR}/src/glew.c
  ${GLEW_SOURCE_DIR}/include
)

target_link_libraries(glew ${GLFW_LIBRARIES})

add_definitions(
  -DGLEW_STATIC
)

add_executable(${PROJECT_NAME} main.cpp compute.cpp)
target_link_libraries(${PROJECT_NAME} glfw ${GLFW_LIBRARIES} glew ${OPENCL_LIBRARIES})

if (APPLE)
  set(APP_NAME "OpenGL Boilerplate")

  # if (EXISTS icon.icns)
    add_executable(${APP_NAME} MACOSX_BUNDLE main.cpp icon.icns)
    set_source_files_properties("icon.icns" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  # else ()
    # add_executable(${APP_NAME} MACOSX_BUNDLE main.cpp)
  # endif ()

  target_link_libraries(${APP_NAME} glfw ${OPENGL_glu_LIBRARY} ${GLFW_LIBRARIES} glew)
  set_target_properties(${APP_NAME} PROPERTIES MACOSX_BUNDLE_ICON_FILE "icon.icns")

  set_target_properties(${APP_NAME} PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME ${APP_NAME})
  set_target_properties(${APP_NAME} PROPERTIES MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0")
  set_target_properties(${APP_NAME} PROPERTIES MACOSX_BUNDLE_LONG_VERSION_STRING "1.0")
  set_target_properties(${APP_NAME} PROPERTIES MACOSX_BUNDLE_INFO_STRING "OpenGL boilerplate example app")
endif()

add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_SOURCE_DIR}/kernels $<TARGET_FILE_DIR:${PROJECT_NAME}>)
