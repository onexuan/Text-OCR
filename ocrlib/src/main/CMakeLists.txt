
cmake_minimum_required(VERSION 2.8.0)

project(ocr)

set(CMAKE_VERBOSE_MAKEFILE on)

set(ANDROID_NDK_ABI_NAME ${CMAKE_ANDROID_ARCH_ABI})

set(cmake_dir ${CMAKE_SOURCE_DIR})

add_definitions(-DOS_ANDROID)
add_definitions(-D__ANDROID__)
add_definitions(-D__ARM_NEON__)
add_definitions(-D__ARM_NEON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -O3 -frtti -fPIC -fpermissive")

# openmp
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fopenmp")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fopenmp")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fomit-frame-pointer -fstrict-aliasing -ffast-math")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fomit-frame-pointer -fstrict-aliasing -ffast-math")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden")

include_directories(
    ${cmake_dir}/cpp
    ${cmake_dir}/cpp/ncnn/include/ncnn)
#link_directories(${CMAKE_CURRENT_SOURCE_DIR}/cpp/ncnn/lib/)

add_library(ncnn STATIC IMPORTED)
set_property(
    TARGET
    ncnn
    PROPERTY
    IMPORTED_LOCATION
    ${cmake_dir}/cpp/ncnn/lib/${ANDROID_ABI}/libncnn.a)

file(GLOB_RECURSE src_files ${cmake_dir}/cpp/jni/*.cpp)

add_library(${PROJECT_NAME} SHARED ${src_files})

target_link_libraries(ocr android jnigraphics z m dl log atomic ncnn)
