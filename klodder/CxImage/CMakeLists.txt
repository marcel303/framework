cmake_minimum_required(VERSION 2.6)

set(HAS_LIBCXIMAGE 1 PARENT_SCOPE)
message("setting HAS_LIBCXIMAGE")

# --- dependencies ---

#

project(libcximage)

# --- libcximage ---

file(GLOB source RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cpp" "*.h")
#list(REMOVE_ITEM source Directory.cpp ImageLoader_FreeImage.cpp test_tga.cpp)

add_library(libcximage ${source})

target_compile_definitions(libcximage PRIVATE _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
target_compile_definitions(libcximage PUBLIC $<$<CONFIG:Debug>:DEBUG>)
target_include_directories(libcximage PUBLIC "${PROJECT_SOURCE_DIR}")

if (MSVC)
	set_property(TARGET libcximage APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
