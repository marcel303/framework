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

project(vfxpro)

# --- vfxpro ---

file(GLOB_RECURSE source "*.cpp" "*.h")

file(GLOB_RECURSE source_mediaplayer "mediaplayer/*.*")
list(REMOVE_ITEM source ${source_mediaplayer})
list(REMOVE_ITEM source "${PROJECT_SOURCE_DIR}/mediaplayer/MediaPlayer.cpp")

file(GLOB_RECURSE source_mediaplayer_old "mediaplayer_old/*.*")
#list(REMOVE_ITEM source ${source_mediaplayer_old})
list(REMOVE_ITEM source "${PROJECT_SOURCE_DIR}/mediaplayer_old/MediaPlayer.cpp")

file(GLOB_RECURSE source_ffmpeg "ffmpeg/*.*")
list(REMOVE_ITEM source ${source_ffmpeg})

if (APPLE)
	file(GLOB_RECURSE source_ffmpeg_old "ffmpeg_old/*.*")
	list(REMOVE_ITEM source ${source_ffmpeg_old})

	file(GLOB_RECURSE source_mediaplayer_old "mediaplayer_old/*.*")
	list(REMOVE_ITEM source ${source_mediaplayer_old})
endif (APPLE)

file(GLOB_RECURSE source_osc "ip/*.*" "osc/*.*")
list(REMOVE_ITEM source ${source_osc})

file(GLOB_RECURSE source_ffmpeg_old "ffmpeg_old/*.*")
#list(REMOVE_ITEM source ${source_ffmpeg_old})

set(GAME_FILES
	main.cpp
	effect.h
	effect.cpp
	scene.h
	scene.cpp)
source_group("Game Files" FILES ${GAME_FILES})
source_group("mediaplayer" FILES ${source_mediaplayer})
source_group("mediaplayer_old" FILES ${source_mediaplayer_old})
source_group("ffmpeg" FILES ${source_ffmpeg})
source_group("ffmpeg_old" FILES ${source_ffmpeg_old})
#source_group("osc" FILES ${source_osc})

add_executable(vfxpro ${source})

target_compile_definitions(vfxpro PRIVATE NO_SDL_GLEXT HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX)

target_include_directories(vfxpro PRIVATE "${PROJECT_SOURCE_DIR}")
#target_include_directories(vfxpro PRIVATE "${PROJECT_SOURCE_DIR}/ffmpeg")
target_include_directories(vfxpro PRIVATE "${PROJECT_SOURCE_DIR}/ffmpeg_old")

target_link_libraries(vfxpro PRIVATE framework)
target_link_libraries(vfxpro PRIVATE libosc)
target_link_libraries(vfxpro PRIVATE libgg)

if (MSVC)
	#target_link_libraries(vfxpro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/avcodec.lib)
	#target_link_libraries(vfxpro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/avformat.lib)
	#target_link_libraries(vfxpro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/avutil.lib)
	target_link_libraries(vfxpro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/ffmpeg_old/avcodec-51.lib)
	target_link_libraries(vfxpro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/ffmpeg_old/avformat-50.lib)
	target_link_libraries(vfxpro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/ffmpeg_old/avutil-49.lib)
	target_link_libraries(vfxpro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/Leap.lib)

	target_link_libraries(vfxpro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/FreeImage.lib)
	target_link_libraries(vfxpro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/freetype252.lib)
	target_link_libraries(vfxpro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/glew32.lib)
	target_link_libraries(vfxpro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/OpenAL32.lib)
	target_link_libraries(vfxpro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/SDL2.lib)
	target_link_libraries(vfxpro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/SDL2main.lib)
	target_link_libraries(vfxpro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/XInput9_1_0.lib)
	target_link_libraries(vfxpro PRIVATE Winmm.lib OpenGL32.lib WSock32.lib Ws2_32.lib)
endif (MSVC)

if (APPLE)
	target_compile_definitions(vfxpro PUBLIC MACOS)

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
	
	target_link_libraries(vfxpro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/macos/glew/lib/libGLEW.a)
	target_link_libraries(vfxpro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/macos/lib/libfreeimage.a)
	target_link_libraries(vfxpro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/macos/lib/libfreetype.a)
	target_link_libraries(vfxpro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/macos/lib/libSDL2.a)
	target_link_libraries(vfxpro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/macos/lib/libSDL2main.a)
	target_link_libraries(vfxpro PRIVATE ${COCOA_LIBRARY})
	target_link_libraries(vfxpro PRIVATE ${CARBON_LIBRARY})
	target_link_libraries(vfxpro PRIVATE ${COREAUDIO_LIBRARY})
	target_link_libraries(vfxpro PRIVATE ${COREVIDEO_LIBRARY})
	target_link_libraries(vfxpro PRIVATE ${IOKIT_LIBRARY})
	target_link_libraries(vfxpro PRIVATE ${AUDIOTOOLBOX_LIBRARY})
	target_link_libraries(vfxpro PRIVATE ${AUDIOUNIT_LIBRARY})
	target_link_libraries(vfxpro PRIVATE ${OPENAL_LIBRARY})
	target_link_libraries(vfxpro PRIVATE ${OPENGL_LIBRARY})
	target_link_libraries(vfxpro PRIVATE ${FF_LIBRARY})
	target_link_libraries(vfxpro PRIVATE ${ICONV_LIBRARY})
	target_link_libraries(vfxpro PRIVATE ${BZ2_LIBRARY})
endif (APPLE)

if (MSVC)
	set_property(TARGET vfxpro APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
