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

source_group("sources" FILES ${source})

source_group("3rdparty\\msdfgen" FILES ${source_msdfgen})
source_group("3rdparty\\remotery" FILES ${source_remotery})
source_group("3rdparty\\tinyxml" FILES ${source_tinyxml})
source_group("3rdparty\\utf8rewind" FILES ${source_utf8rewind})

add_library(framework ${source})

target_compile_definitions(framework PUBLIC NO_SDL_GLEXT)
target_compile_definitions(framework PUBLIC FRAMEWORK_USE_OPENAL=0)
target_compile_definitions(framework PUBLIC FRAMEWORK_USE_PORTAUDIO=1)
target_compile_definitions(framework PUBLIC $<$<CONFIG:Debug>:DEBUG>)

target_link_libraries(framework libgg)

if (MSVC)
	target_compile_definitions(framework PUBLIC HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
endif (MSVC)

target_include_directories(framework PUBLIC "${PROJECT_SOURCE_DIR}")
target_include_directories(framework PRIVATE "${PROJECT_SOURCE_DIR}/audiostream") # for ogg and vorbis headers

if (WIN32)
	target_include_directories(framework PUBLIC "${PROJECT_SOURCE_DIR}/win32/include")
	target_include_directories(framework PRIVATE "${PROJECT_SOURCE_DIR}/win32/include/freetype")
endif (WIN32)

if (APPLE)
	target_include_directories(framework PUBLIC "${PROJECT_SOURCE_DIR}/macos/include")
	target_include_directories(framework PRIVATE "${PROJECT_SOURCE_DIR}/macos/include/freetype")
endif (APPLE)

if (WIN32)
	set(FRAMEWORK_LIBRARIES
		framework
		libgg
		${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/FreeImage.lib
		${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/freetype252.lib
		${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/glew32.lib
		${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/OpenAL32.lib
		${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/SDL2.lib
		${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/SDL2main.lib
		${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/XInput9_1_0.lib
		${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/portaudio_x86.lib
		Winmm.lib
		OpenGL32.lib
		WSock32.lib
		PARENT_SCOPE)

	set(FRAMEWORK_BINARIES
		${CMAKE_CURRENT_SOURCE_DIR}/win32/dll/FreeImage.dll
		${CMAKE_CURRENT_SOURCE_DIR}/win32/dll/glew32.dll
		${CMAKE_CURRENT_SOURCE_DIR}/win32/dll/OpenAL32.dll
		${CMAKE_CURRENT_SOURCE_DIR}/win32/dll/SDL2.dll
		${CMAKE_CURRENT_SOURCE_DIR}/win32/dll/XInput9_1_0.dll
		${CMAKE_CURRENT_SOURCE_DIR}/win32/dll/portaudio_x86.dll
		PARENT_SCOPE)
endif (WIN32)

if (APPLE)
	find_library(COCOA_LIBRARY Cocoa)
	find_library(CARBON_LIBRARY Carbon)
	find_library(COREAUDIO_LIBRARY CoreAudio)
	find_library(COREVIDEO_LIBRARY CoreVideo)
	find_library(IOKIT_LIBRARY IOKit)
	find_library(AUDIOTOOLBOX_LIBRARY AudioToolbox)
	find_library(OPENGL_LIBRARY OpenGL)
	find_library(FF_LIBRARY ForceFeedback)
	find_library(ICONV_LIBRARY iconv)
	find_library(FREETYPE_LIBRARY freetype2)
	find_library(BZ2_LIBRARY bz2)

	# these libraries are only needed for OpenAL
	#find_library(AUDIOUNIT_LIBRARY AudioUnit)
	#find_library(OPENAL_LIBRARY OpenAL)

	set(FRAMEWORK_LIBRARIES
		framework
		libgg
		${CMAKE_CURRENT_SOURCE_DIR}/macos/glew/lib/libGLEW.a
		${CMAKE_CURRENT_SOURCE_DIR}/macos/lib/libfreeimage.a
		${CMAKE_CURRENT_SOURCE_DIR}/macos/lib/libfreetype.a
		${CMAKE_CURRENT_SOURCE_DIR}/macos/lib/libSDL2.a
		${CMAKE_CURRENT_SOURCE_DIR}/macos/lib/libSDL2main.a
		${CMAKE_CURRENT_SOURCE_DIR}/macos/lib/libportaudio.a
		${COCOA_LIBRARY}
		${CARBON_LIBRARY}
		${COREAUDIO_LIBRARY}
		${COREVIDEO_LIBRARY}
		${IOKIT_LIBRARY}
		${AUDIOTOOLBOX_LIBRARY}
		${OPENGL_LIBRARY}
		${FF_LIBRARY}
		${ICONV_LIBRARY}
		${BZ2_LIBRARY}
		PARENT_SCOPE)

	set(FRAMEWORK_BINARIES
		PARENT_SCOPE)
endif (APPLE)

if (MSVC)
	set_property(TARGET framework APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
