cmake_minimum_required(VERSION 2.6)

set(HAS_LIBAGG 1 PARENT_SCOPE)
message("setting HAS_LIBAGG")

# --- dependencies ---

#

project(libagg)

# --- libagg ---

file(GLOB source RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cpp" "*.h")
#list(REMOVE_ITEM source Directory.cpp ImageLoader_FreeImage.cpp test_tga.cpp)

add_library(libagg ${source})

target_compile_definitions(libagg PRIVATE _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
target_compile_definitions(libagg PUBLIC $<$<CONFIG:Debug>:DEBUG>)
target_include_directories(libagg PUBLIC "${PROJECT_SOURCE_DIR}")

if (MSVC)
	set_property(TARGET libagg APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
