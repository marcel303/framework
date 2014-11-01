cmake_minimum_required(VERSION 2.6)

project(libgg)

# --- libgg ---

file(GLOB source RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
list(REMOVE_ITEM source Directory.cpp ImageLoader_FreeImage.cpp)

add_library(libgg ${source})

target_compile_definitions(libgg PRIVATE _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
target_include_directories(libgg PUBLIC "${PROJECT_SOURCE_DIR}")