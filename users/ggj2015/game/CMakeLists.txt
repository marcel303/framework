cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 14)

set(HAS_LIBGG 0)
set(HAS_FRAMEWORK 0)

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../../../libgg libgg)
endif (NOT HAS_LIBGG)
if (NOT HAS_FRAMEWORK)
	message("including framework")
	add_subdirectory(../../../framework framework)
endif (NOT HAS_FRAMEWORK)

#

project(game)

# --- game ---

file(GLOB source "*.cpp" "*.h")

add_executable(game ${source})

target_compile_definitions(game PRIVATE NO_SDL_GLEXT HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)

target_include_directories(game PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(game PRIVATE framework)
target_link_libraries(game PRIVATE libgg)

if (MSVC)
	target_link_libraries(game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../framework/win32/lib/FreeImage.lib)
	target_link_libraries(game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../framework/win32/lib/freetype252.lib)
	target_link_libraries(game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../framework/win32/lib/glew32.lib)
	target_link_libraries(game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../framework/win32/lib/OpenAL32.lib)
	target_link_libraries(game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../framework/win32/lib/SDL2.lib)
	target_link_libraries(game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../framework/win32/lib/SDL2main.lib)
	target_link_libraries(game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../framework/win32/lib/XInput9_1_0.lib)
	target_link_libraries(game PRIVATE Winmm.lib OpenGL32.lib WSock32.lib Ws2_32.lib)
endif (MSVC)

if (APPLE)
	target_compile_definitions(game PUBLIC MACOS)

	find_library(COCOA_LIBRARY Cocoa)
	find_library(CARBON_LIBRARY Carbon)
	find_library(COREAUDIO_LIBRARY CoreAudio)
	find_library(COREVIDEO_LIBRARY CoreVideo)
	find_library(COREMEDIA_LIBRARY CoreMedia)
	find_library(IOKIT_LIBRARY IOKit)
	find_library(AUDIOTOOLBOX_LIBRARY AudioToolbox)
	find_library(AUDIOUNIT_LIBRARY AudioUnit)
	find_library(VIDEOTOOLBOX_LIBRARY VideoToolbox)
	find_library(VDA_LIBRARY VideoDecodeAcceleration)
	find_library(AVFOUNDATION_LIBRARY AVFoundation)
	find_library(OPENAL_LIBRARY OpenAL)
	find_library(OPENGL_LIBRARY OpenGL)
	find_library(FF_LIBRARY ForceFeedback)
	find_library(ICONV_LIBRARY iconv)
	find_library(FREETYPE_LIBRARY freetype2)
	find_library(BZ2_LIBRARY bz2)
	find_library(SECURITY_LIBRARY Security)
	
	target_link_libraries(game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../framework/macos/glew/lib/libGLEW.a)
	target_link_libraries(game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../framework/macos/lib/libfreeimage.a)
	target_link_libraries(game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../framework/macos/lib/libfreetype.a)
	target_link_libraries(game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../framework/macos/lib/libSDL2.a)
	target_link_libraries(game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../framework/macos/lib/libSDL2main.a)
	target_link_libraries(game PRIVATE ${COCOA_LIBRARY})
	target_link_libraries(game PRIVATE ${CARBON_LIBRARY})
	target_link_libraries(game PRIVATE ${COREAUDIO_LIBRARY})
	target_link_libraries(game PRIVATE ${COREVIDEO_LIBRARY})
	target_link_libraries(game PRIVATE ${COREMEDIA_LIBRARY})
	target_link_libraries(game PRIVATE ${IOKIT_LIBRARY})
	target_link_libraries(game PRIVATE ${AUDIOTOOLBOX_LIBRARY})
	target_link_libraries(game PRIVATE ${AUDIOUNIT_LIBRARY})
	target_link_libraries(game PRIVATE ${VIDEOTOOLBOX_LIBRARY})
	target_link_libraries(game PRIVATE ${VDA_LIBRARY})
	target_link_libraries(game PRIVATE ${AVFOUNDATION_LIBRARY})
	target_link_libraries(game PRIVATE ${OPENAL_LIBRARY})
	target_link_libraries(game PRIVATE ${OPENGL_LIBRARY})
	target_link_libraries(game PRIVATE ${FF_LIBRARY})
	target_link_libraries(game PRIVATE ${ICONV_LIBRARY})
	target_link_libraries(game PRIVATE ${BZ2_LIBRARY})
	target_link_libraries(game PRIVATE ${SECURITY_LIBRARY})
endif (APPLE)

if (MSVC)
	set_property(TARGET game APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
