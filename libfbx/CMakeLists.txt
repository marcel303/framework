cmake_minimum_required(VERSION 2.6)

#

project(fbxtest)

# --- fbxtest ---

file(GLOB_RECURSE source "*.cpp" "*.h")

add_executable(fbxtest ${source})

target_compile_definitions(fbxtest PRIVATE NO_SDL_GLEXT HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX)

target_include_directories(fbxtest PRIVATE "${PROJECT_SOURCE_DIR}")

if (MSVC)
	set_property(TARGET fbxtest APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
