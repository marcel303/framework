cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 11)

set(HAS_LIBGG 0)
set(HAS_LIBOSC 0)
set(HAS_FRAMEWORK 0)

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../../../libgg libgg)
endif (NOT HAS_LIBGG)
if (NOT HAS_LIBOSC)
	message("including libosc")
	add_subdirectory(../../../libosc libosc)
endif (NOT HAS_LIBOSC)
if (NOT HAS_FRAMEWORK)
	message("including framework")
	add_subdirectory(../framework framework)
endif (NOT HAS_FRAMEWORK)

#

project(vfxedit)

# --- vfxedit ---

file(GLOB_RECURSE source "*.cpp" "*.h")

file(GLOB_RECURSE source_midifile "midifile/*.*")
source_group("midifile" FILES ${source_midifile})

add_executable(vfxedit ${source})

target_compile_definitions(vfxedit PRIVATE NO_SDL_GLEXT HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX)

target_include_directories(vfxedit PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(vfxedit PRIVATE framework)
target_link_libraries(vfxedit PRIVATE libosc)
target_link_libraries(vfxedit PRIVATE libgg)

if (MSVC)
	target_link_libraries(vfxedit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/FreeImage.lib)
	target_link_libraries(vfxedit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/freetype252.lib)
	target_link_libraries(vfxedit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/glew32.lib)
	target_link_libraries(vfxedit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/OpenAL32.lib)
	target_link_libraries(vfxedit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/SDL2.lib)
	target_link_libraries(vfxedit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/SDL2main.lib)
	target_link_libraries(vfxedit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/XInput9_1_0.lib)
	target_link_libraries(vfxedit PRIVATE Winmm.lib OpenGL32.lib WSock32.lib Ws2_32.lib)
endif (MSVC)

if (APPLE)
	target_compile_definitions(vfxedit PUBLIC MACOS)

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
	
	target_link_libraries(vfxedit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/macos/glew/lib/libGLEW.a)
	target_link_libraries(vfxedit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/macos/lib/libfreeimage.a)
	target_link_libraries(vfxedit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/macos/lib/libfreetype.a)
	target_link_libraries(vfxedit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/macos/lib/libSDL2.a)
	target_link_libraries(vfxedit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/macos/lib/libSDL2main.a)
	target_link_libraries(vfxedit PRIVATE ${COCOA_LIBRARY})
	target_link_libraries(vfxedit PRIVATE ${CARBON_LIBRARY})
	target_link_libraries(vfxedit PRIVATE ${COREAUDIO_LIBRARY})
	target_link_libraries(vfxedit PRIVATE ${COREVIDEO_LIBRARY})
	target_link_libraries(vfxedit PRIVATE ${IOKIT_LIBRARY})
	target_link_libraries(vfxedit PRIVATE ${AUDIOTOOLBOX_LIBRARY})
	target_link_libraries(vfxedit PRIVATE ${AUDIOUNIT_LIBRARY})
	target_link_libraries(vfxedit PRIVATE ${OPENAL_LIBRARY})
	target_link_libraries(vfxedit PRIVATE ${OPENGL_LIBRARY})
	target_link_libraries(vfxedit PRIVATE ${FF_LIBRARY})
	target_link_libraries(vfxedit PRIVATE ${ICONV_LIBRARY})
	target_link_libraries(vfxedit PRIVATE ${BZ2_LIBRARY})
endif (APPLE)

if (MSVC)
	set_property(TARGET vfxedit APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
