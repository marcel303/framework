cmake_minimum_required(VERSION 2.6)

set(HAS_LIBKLODDER 1 PARENT_SCOPE)
message("setting HAS_LIBKLODDER")

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../../libgg libgg)
endif (NOT HAS_LIBGG)

if (NOT HAS_LIBAGG)
	message("including libagg")
	add_subdirectory(../libagg libagg)
endif (NOT HAS_LIBAGG)

if (NOT HAS_LIBCXIMAGE)
	message("including libcximage")
	add_subdirectory(../CxImage libcximage)
endif (NOT HAS_LIBCXIMAGE)

#

project(libklodder)

# --- libklodder ---

set(source ../Classes/BlitTransform.cpp ../Classes/BrushLibrary.cpp ../Classes/Brush_Pattern.cpp ../Classes/Filter.cpp ../Classes/ImageResampling.cpp ../Classes/MacImage.cpp ../Classes/BlitTransform.h ../Classes/BrushLibrary.h ../Classes/Brush_Pattern.h ../Classes/Filter.h ../Classes/ImageResampling.h ../Classes/MacImage.h)

add_library(libklodder ${source})

target_compile_definitions(libklodder PRIVATE _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
target_compile_definitions(libklodder PUBLIC $<$<CONFIG:Debug>:DEBUG>)
target_include_directories(libklodder PUBLIC "${PROJECT_SOURCE_DIR}")
target_include_directories(libklodder PUBLIC "${PROJECT_SOURCE_DIR}/../../libgg") # because CMake sucks and only imports the PUBLIC include directories if you're DIRECTLY linking
target_include_directories(libklodder PUBLIC "${PROJECT_SOURCE_DIR}/../libagg") # because CMake sucks and only imports the PUBLIC include directories if you're DIRECTLY linking against a library
target_include_directories(libklodder PUBLIC "${PROJECT_SOURCE_DIR}/../CxImage") # because CMake sucks and only imports the PUBLIC include directories if you're DIRECTLY linking against a library

add_dependencies(libklodder libgg)
add_dependencies(libklodder libagg)
add_dependencies(libklodder libcximage)

if (MSVC)
	set_property(TARGET libklodder APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
