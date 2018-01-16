cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 14)

set(HAS_LIBVIDEO 1 PARENT_SCOPE)
message("setting HAS_LIBVIDEO")

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../libgg libgg)
endif (NOT HAS_LIBGG)
if (NOT HAS_FRAMEWORK)
	message("including framework")
	add_subdirectory(../framework framework)
endif (NOT HAS_FRAMEWORK)

#

project(libvideo)

# --- libvideo ---

file(GLOB_RECURSE source_mediaplayer "mediaplayer/*.*")
file(GLOB_RECURSE source_video "video.cpp" "video.h")

list(APPEND source ${source_mediaplayer})
list(APPEND source ${source_video})

source_group("mediaplayer" FILES ${source_mediaplayer})
source_group("video" FILES ${source_video})

add_library(libvideo ${source})

target_include_directories(libvideo PUBLIC "${PROJECT_SOURCE_DIR}")

target_link_libraries(libvideo libgg)
target_link_libraries(libvideo framework)

if (WIN32)
	target_include_directories(libvideo PUBLIC "${PROJECT_SOURCE_DIR}/win32/include")
endif (WIN32)

if (APPLE)
	target_include_directories(libvideo PUBLIC "${PROJECT_SOURCE_DIR}/macos/include")
endif (APPLE)

if (WIN32)
	target_link_libraries(libvideo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/avcodec.lib)
	target_link_libraries(libvideo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/avdevice.lib)
	target_link_libraries(libvideo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/avfilter.lib)
	target_link_libraries(libvideo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/avformat.lib)
	target_link_libraries(libvideo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/avutil.lib)
	target_link_libraries(libvideo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/swresample.lib)
	target_link_libraries(libvideo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/swscale.lib)

	set(LIBVIDEO_LIBRARIES
		PARENT_SCOPE)

	set(LIBVIDEO_BINARIES
		${CMAKE_CURRENT_SOURCE_DIR}/win32/dll/avcodec-57.dll
		${CMAKE_CURRENT_SOURCE_DIR}/win32/dll/avdevice-57.dll
		${CMAKE_CURRENT_SOURCE_DIR}/win32/dll/avfilter-6.dll
		${CMAKE_CURRENT_SOURCE_DIR}/win32/dll/avformat-57.dll
		${CMAKE_CURRENT_SOURCE_DIR}/win32/dll/avutil-55.dll
		${CMAKE_CURRENT_SOURCE_DIR}/win32/dll/postproc-54.dll
		${CMAKE_CURRENT_SOURCE_DIR}/win32/dll/swresample-2.dll
		${CMAKE_CURRENT_SOURCE_DIR}/win32/dll/swscale-4.dll
		PARENT_SCOPE)
endif (WIN32)

if (APPLE)
	find_library(COREMEDIA_LIBRARY CoreMedia)
	find_library(VDA_LIBRARY VideoDecodeAcceleration)
	find_library(VIDEOTOOLBOX_LIBRARY VideoToolbox)
	find_library(SECURITY_LIBRARY Security)

	set(LIBVIDEO_LIBRARIES
		${CMAKE_CURRENT_SOURCE_DIR}/macos/lib/libavcodec.a
		${CMAKE_CURRENT_SOURCE_DIR}/macos/lib/libavdevice.a
		${CMAKE_CURRENT_SOURCE_DIR}/macos/lib/libavfilter.a
		${CMAKE_CURRENT_SOURCE_DIR}/macos/lib/libavformat.a
		${CMAKE_CURRENT_SOURCE_DIR}/macos/lib/libavutil.a
		${CMAKE_CURRENT_SOURCE_DIR}/macos/lib/libswresample.a
		${CMAKE_CURRENT_SOURCE_DIR}/macos/lib/libswscale.a
		${COREMEDIA_LIBRARY}
		${VDA_LIBRARY}
		${VIDEOTOOLBOX_LIBRARY}
		${SECURITY_LIBRARY}
		PARENT_SCOPE)
endif (APPLE)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	find_library(AVCODEC_LIBRARY avcodec)
	find_library(AVDEVICE_LIBRARY avdevice)
	find_library(AVFILTER_LIBRARY avfilter)
	find_library(AVFORMAT_LIBRARY avformat)
	find_library(AVUTIL_LIBRARY avutil)
	find_library(SWSCALE_LIBRARY swscale)
	
	set(LIBVIDEO_LIBRARIES
		${AVCODEC_LIBRARY}
		${AVDEVICE_LIBRARY}
		${AVFILTER_LIBRARY}
		${AVFORMAT_LIBRARY}
		${AVUTIL_LIBRARY}
		${SWSCALE_LIBRARY}
		PARENT_SCOPE)
endif ()
