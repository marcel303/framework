cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 11)

set(HAS_LIBGG 0)
set(HAS_LIBOSC 0)
set(HAS_FRAMEWORK 0)
set(HAS_LIBVIDEO 0)

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
	add_subdirectory(../../../framework framework)
endif (NOT HAS_FRAMEWORK)
if (NOT HAS_LIBVIDEO)
	message("including libvideo")
	add_subdirectory(../../../libvideo libvideo)
endif (NOT HAS_LIBVIDEO)

#

project(avpaint)

# --- avpaint ---

file(GLOB_RECURSE source "*.cpp" "*.h")

add_executable(avpaint ${source})

target_include_directories(avpaint PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(avpaint PRIVATE framework)
target_link_libraries(avpaint PRIVATE libosc)
target_link_libraries(avpaint PRIVATE libgg)
target_link_libraries(avpaint PRIVATE libvideo)

target_link_libraries(avpaint PRIVATE ${FRAMEWORK_LIBRARIES})
target_link_libraries(avpaint PRIVATE ${LIBVIDEO_LIBRARIES})

if (MSVC)
	target_link_libraries(avpaint PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../vfxGraph/win32/lib/Leap.lib)
	target_link_libraries(avpaint PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../vfxGraph/win32/lib/portaudio_x86.lib)
endif (MSVC)

if (APPLE)
	target_include_directories(avpaint PRIVATE "../../../vfxGraph/macos/include")

	target_compile_definitions(avpaint PUBLIC MACOS)

	target_link_libraries(avpaint PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../vfxGraph/macos/lib/libLeap.dylib)
	target_link_libraries(avpaint PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../vfxGraph/macos/lib/libportaudio.a)
	target_link_libraries(avpaint PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../vfxGraph/macos/lib/libturbojpeg.0.dylib)
endif (APPLE)

if (MSVC)
	set_property(TARGET avpaint APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
	set_property(TARGET avpaint APPEND_STRING PROPERTY LINK_FLAGS /SAFESEH:NO)
endif (MSVC)
