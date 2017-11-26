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

file(GLOB_RECURSE source RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(libvideo ${source})

target_compile_definitions(libvideo PRIVATE NO_SDL_GLEXT)

if (MSVC)
	target_compile_definitions(libvideo PRIVATE HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
endif (MSVC)

target_include_directories(libvideo PUBLIC "${PROJECT_SOURCE_DIR}")

target_include_directories(libvideo PUBLIC "${PROJECT_SOURCE_DIR}/../libgg") # because CMake sucks and only imports the PUBLIC include directories if you're DIRECTLY linking against a library
target_include_directories(libvideo PUBLIC "${PROJECT_SOURCE_DIR}/../framework") # because CMake sucks and only imports the PUBLIC include directories if you're DIRECTLY linking against a library

if (APPLE)
	target_include_directories(libvideo PUBLIC "${PROJECT_SOURCE_DIR}/../framework/macos/include") # because CMake sucks and only imports the PUBLIC include directories if you're DIRECTLY linking against a library

	target_include_directories(libvideo PRIVATE "${PROJECT_SOURCE_DIR}/macos/include")
endif (APPLE)

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

add_dependencies(libvideo libgg)
add_dependencies(libvideo framework)
