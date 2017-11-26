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

project(anaglyphic)

# --- anaglyphic ---

file(GLOB_RECURSE source "*.c" "*.cpp" "*.h")
file(GLOB_RECURSE source_video "../../../libvideo/video.cpp" "../../../libvideo/video.h")
file(GLOB_RECURSE source_mediaplayer "../../../libvideo/mediaplayer/*.*")

list(APPEND source ${source_video})
list(APPEND source ${source_mediaplayer})

add_executable(anaglyphic ${source})

target_compile_definitions(anaglyphic PRIVATE NO_SDL_GLEXT HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX)

target_include_directories(anaglyphic PRIVATE "${PROJECT_SOURCE_DIR}")
target_include_directories(anaglyphic PRIVATE "../../../libvideo/")

if (APPLE)
	target_include_directories(anaglyphic PRIVATE "../../../libvideo/macos/include")
endif (APPLE)

target_link_libraries(anaglyphic PRIVATE framework)
target_link_libraries(anaglyphic PRIVATE libgg)

if (MSVC)
	target_link_libraries(anaglyphic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/ffmpeg/avcodec-51.lib)
	target_link_libraries(anaglyphic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/ffmpeg/avformat-50.lib)
	target_link_libraries(anaglyphic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/ffmpeg/avutil-49.lib)

	target_link_libraries(anaglyphic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/FreeImage.lib)
	target_link_libraries(anaglyphic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/freetype252.lib)
	target_link_libraries(anaglyphic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/glew32.lib)
	target_link_libraries(anaglyphic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/OpenAL32.lib)
	target_link_libraries(anaglyphic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/SDL2.lib)
	target_link_libraries(anaglyphic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/SDL2main.lib)
	target_link_libraries(anaglyphic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/XInput9_1_0.lib)
	target_link_libraries(anaglyphic PRIVATE Winmm.lib OpenGL32.lib WSock32.lib Ws2_32.lib)
endif (MSVC)

if (APPLE)
	find_library(COREMEDIA_LIBRARY CoreMedia)
	find_library(VDA_LIBRARY VideoDecodeAcceleration)
	find_library(VIDEOTOOLBOX_LIBRARY VideoToolbox)
	find_library(SECURITY_LIBRARY Security)

	set(LIBVIDEO_LIBRARIES ${COREMEDIA_LIBRARY} ${VDA_LIBRARY} ${VIDEOTOOLBOX_LIBRARY} ${SECURITY_LIBRARY})

	target_link_libraries(anaglyphic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../libvideo/macos/lib/libavcodec.a)
	target_link_libraries(anaglyphic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../libvideo/macos/lib/libavdevice.a)
	target_link_libraries(anaglyphic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../libvideo/macos/lib/libavfilter.a)
	target_link_libraries(anaglyphic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../libvideo/macos/lib/libavformat.a)
	target_link_libraries(anaglyphic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../libvideo/macos/lib/libavutil.a)
	target_link_libraries(anaglyphic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../libvideo/macos/lib/libswresample.a)
	target_link_libraries(anaglyphic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../libvideo/macos/lib/libswscale.a)

	target_link_libraries(anaglyphic PRIVATE ${FRAMEWORK_LIBRARIES})
	target_link_libraries(anaglyphic PRIVATE ${LIBVIDEO_LIBRARIES})
endif (APPLE)

if (MSVC)
	set_property(TARGET anaglyphic APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
