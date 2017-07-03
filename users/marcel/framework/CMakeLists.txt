cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 14)

set(HAS_FRAMEWORK 1 PARENT_SCOPE)
message("setting HAS_FRAMEWORK")

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../../../libgg libgg)
endif (NOT HAS_LIBGG)

#

project(framework)

# --- framework ---

file(GLOB source RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/audiostream/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/audiostream/*.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/audiostream/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/msdfgen/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/msdfgen/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/../fbx1/*.cpp")
list(REMOVE_ITEM source aniplay.cpp)
list(REMOVE_ITEM source example.cpp)
list(REMOVE_ITEM source ../fbx1/main.cpp)

if (APPLE)
	#list(REMOVE_ITEM source remotery.cpp)
endif (APPLE)

add_library(framework ${source})

target_compile_definitions(framework PRIVATE NO_SDL_GLEXT)
target_compile_definitions(framework PUBLIC $<$<CONFIG:Debug>:DEBUG>)

if (MSVC)
	target_compile_definitions(framework PRIVATE HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
endif (MSVC)

if (APPLE)
	target_compile_definitions(framework PUBLIC MACOS)
endif (APPLE)

target_include_directories(framework PUBLIC "${PROJECT_SOURCE_DIR}")
target_include_directories(framework PRIVATE "${PROJECT_SOURCE_DIR}/audiostream")
target_include_directories(framework PUBLIC "${PROJECT_SOURCE_DIR}/../../../libgg") # because CMake sucks and only imports the PUBLIC include directories if you're DIRECTLY linking against a library

if (MSVC)
	target_include_directories(framework PUBLIC "${PROJECT_SOURCE_DIR}/win32/include")
	target_include_directories(framework PRIVATE "${PROJECT_SOURCE_DIR}/win32/include/freetype")
endif (MSVC)

if (APPLE)
	target_include_directories(framework PUBLIC "${PROJECT_SOURCE_DIR}/macos/include")
	target_include_directories(framework PRIVATE "${PROJECT_SOURCE_DIR}/macos/include/freetype")
endif (APPLE)

add_dependencies(framework libgg)

if (MSVC)
	set_property(TARGET framework APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
