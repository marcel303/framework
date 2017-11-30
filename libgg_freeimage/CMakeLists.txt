cmake_minimum_required(VERSION 2.6)

set(HAS_LIBGG_FREEIMAGE 1 PARENT_SCOPE)
message("setting HAS_LIBGG_FREEIMAGE")

# --- dependencies ---

#

project(libgg_freeimage)

# --- libgg_freeimage ---

set(source ../libgg/ImageLoader_FreeImage.cpp)

source_group("sources" FILES ${source})

add_library(libgg_freeimage ${source})

target_compile_definitions(libgg_freeimage PRIVATE _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
target_compile_definitions(libgg_freeimage PUBLIC $<$<CONFIG:Debug>:DEBUG>)
target_include_directories(libgg_freeimage PUBLIC "${PROJECT_SOURCE_DIR}")

if (MSVC)
	set_property(TARGET libgg_freeimage APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
