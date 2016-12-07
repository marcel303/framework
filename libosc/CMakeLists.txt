cmake_minimum_required(VERSION 2.6)

set(HAS_LIBOSC 1 PARENT_SCOPE)
message("setting HAS_LIBOSC")

# --- dependencies ---

#

project(libosc)

# --- libosc ---

file(GLOB source RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.c" "*.cpp" "*.h")

add_library(libosc ${source})

target_compile_definitions(libosc PRIVATE _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
target_include_directories(libosc PUBLIC "${PROJECT_SOURCE_DIR}")

if (MSVC)
	set_property(TARGET libosc APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
