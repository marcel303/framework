cmake_minimum_required(VERSION 2.6)

#

project(fbxtest)

# --- fbxtest ---

file(GLOB_RECURSE source "*.cpp" "*.h")

source_group("sources" FILES ${source})

add_executable(fbxtest ${source})

if (MSVC)
	target_compile_definitions(fbxtest PRIVATE HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX)
endif (MSVC)

target_include_directories(fbxtest PRIVATE "${PROJECT_SOURCE_DIR}")

if (MSVC)
	set_property(TARGET fbxtest APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
