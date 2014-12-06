cmake_minimum_required(VERSION 2.6)

set(HAS_LIBGG 1 PARENT_SCOPE)
message("setting HAS_LIBGG")

# --- dependencies ---

#

project(libgg)

# --- libgg ---

file(GLOB source RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cpp" "*.h")
list(REMOVE_ITEM source Directory.cpp ImageLoader_FreeImage.cpp test_tga.cpp)

add_library(libgg ${source})

target_compile_definitions(libgg PRIVATE _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
target_compile_definitions(libgg PUBLIC DEBUG)
target_include_directories(libgg PUBLIC "${PROJECT_SOURCE_DIR}")

if (MSVC)
	set_property(TARGET libgg APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
