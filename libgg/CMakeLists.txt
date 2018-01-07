cmake_minimum_required(VERSION 2.6)

set(HAS_LIBGG 1 PARENT_SCOPE)
message("setting HAS_LIBGG")

# --- dependencies ---

#

project(libgg)

# --- libgg ---

file(GLOB_RECURSE source RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cpp" "*.h")
list(REMOVE_ITEM source Directory.cpp ImageLoader_FreeImage.cpp test_tga.cpp)

source_group("sources" FILES ${source})

add_library(libgg ${source})

target_compile_definitions(libgg PUBLIC $<$<CONFIG:Debug>:DEBUG>)
target_include_directories(libgg PUBLIC "${PROJECT_SOURCE_DIR}")

if (WIN32)
	target_compile_definitions(libgg PUBLIC WINDOWS)
endif (WIN32)

if (MSVC)
	target_compile_definitions(libgg PUBLIC _USE_MATH_DEFINES)
	target_compile_definitions(libgg PRIVATE _CRT_SECURE_NO_WARNINGS)
	set_property(TARGET libgg APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)

if (APPLE)
	target_compile_definitions(libgg PUBLIC MACOS)
endif (APPLE)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	target_compile_definitions(libgg PUBLIC LINUX)
endif ()
