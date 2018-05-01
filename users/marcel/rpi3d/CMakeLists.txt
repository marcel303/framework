cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 14)

project(rpi3d)

add_executable(rpi3d main.cpp)

include(FindPkgConfig)
find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)

target_link_libraries(rpi3d ${OPENGL_LIBRARIES} ${SDL2_LIBRARIES})

