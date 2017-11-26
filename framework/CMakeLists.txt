cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 14)

set(HAS_FRAMEWORK 1 PARENT_SCOPE)
message("setting HAS_FRAMEWORK")

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../libgg libgg)
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
	"${CMAKE_CURRENT_SOURCE_DIR}/../libfbx/*.cpp")
list(REMOVE_ITEM source ../libfbx/main.cpp)

file(GLOB_RECURSE source_msdfgen "msdfgen/*.*")
file(GLOB_RECURSE source_remotery "remotery*.*")
file(GLOB_RECURSE source_tinyxml "tinyxml*.*")
file(GLOB_RECURSE source_utf8rewind "utf8rewind*.*")

source_group("3rdparty\\msdfgen" FILES ${source_msdfgen})
source_group("3rdparty\\remotery" FILES ${source_remotery})
source_group("3rdparty\\tinyxml" FILES ${source_tinyxml})
source_group("3rdparty\\utf8rewind" FILES ${source_utf8rewind})

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
target_include_directories(framework PUBLIC "${PROJECT_SOURCE_DIR}/../libgg") # because CMake sucks and only imports the PUBLIC include directories if you're DIRECTLY linking against a library

if (MSVC)
	target_include_directories(framework PUBLIC "${PROJECT_SOURCE_DIR}/win32/include")
	target_include_directories(framework PRIVATE "${PROJECT_SOURCE_DIR}/win32/include/freetype")
endif (MSVC)

if (APPLE)
	target_include_directories(framework PUBLIC "${PROJECT_SOURCE_DIR}/macos/include")
	target_include_directories(framework PRIVATE "${PROJECT_SOURCE_DIR}/macos/include/freetype")
endif (APPLE)

if (APPLE)
	find_library(COCOA_LIBRARY Cocoa)
	find_library(CARBON_LIBRARY Carbon)
	find_library(COREAUDIO_LIBRARY CoreAudio)
	find_library(COREVIDEO_LIBRARY CoreVideo)
	find_library(IOKIT_LIBRARY IOKit)
	find_library(AUDIOTOOLBOX_LIBRARY AudioToolbox)
	find_library(AUDIOUNIT_LIBRARY AudioUnit)
	find_library(OPENAL_LIBRARY OpenAL)
	find_library(OPENGL_LIBRARY OpenGL)
	find_library(FF_LIBRARY ForceFeedback)
	find_library(ICONV_LIBRARY iconv)
	find_library(FREETYPE_LIBRARY freetype2)
	find_library(BZ2_LIBRARY bz2)

	set(FRAMEWORK_LIBRARIES
		framework
		libgg
		${CMAKE_CURRENT_SOURCE_DIR}/macos/glew/lib/libGLEW.a
		${CMAKE_CURRENT_SOURCE_DIR}/macos/lib/libfreeimage.a
		${CMAKE_CURRENT_SOURCE_DIR}/macos/lib/libfreetype.a
		${CMAKE_CURRENT_SOURCE_DIR}/macos/lib/libSDL2.a
		${CMAKE_CURRENT_SOURCE_DIR}/macos/lib/libSDL2main.a
		${COCOA_LIBRARY}
		${CARBON_LIBRARY}
		${COREAUDIO_LIBRARY}
		${COREVIDEO_LIBRARY}
		${IOKIT_LIBRARY}
		${AUDIOTOOLBOX_LIBRARY}
		${AUDIOUNIT_LIBRARY}
		${OPENAL_LIBRARY}
		${OPENGL_LIBRARY}
		${FF_LIBRARY}
		${ICONV_LIBRARY}
		${BZ2_LIBRARY}
		PARENT_SCOPE)
endif (APPLE)

add_dependencies(framework libgg)

if (MSVC)
	set_property(TARGET framework APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
