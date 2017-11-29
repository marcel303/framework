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

project(vfxpro)

# --- vfxpro ---

file(GLOB_RECURSE source "*.cpp" "*.h")

set(GAME_FILES
	main.cpp
	effect.h
	effect.cpp
	scene.h
	scene.cpp)
source_group("Game Files" FILES ${GAME_FILES})
source_group("include" FILES ${source_macos})

add_executable(vfxpro ${source})

target_include_directories(vfxpro PRIVATE "${PROJECT_SOURCE_DIR}")

if (MSVC)
	target_include_directories(vfxpro PRIVATE "${PROJECT_SOURCE_DIR}/win32/include")
endif (MSVC)
if (APPLE)
	target_include_directories(vfxpro PRIVATE "${PROJECT_SOURCE_DIR}/../../../vfxGraph/macos/include")
endif (APPLE)

target_link_libraries(vfxpro PRIVATE framework)
target_link_libraries(vfxpro PRIVATE libosc)
target_link_libraries(vfxpro PRIVATE libgg)
target_link_libraries(vfxpro PRIVATE libvideo)

target_link_libraries(vfxpro PRIVATE ${FRAMEWORK_LIBRARIES})
target_link_libraries(vfxpro PRIVATE ${LIBVIDEO_LIBRARIES})

if (MSVC)
	target_link_libraries(vfxpro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../vfxGraph/win32/lib/Leap.lib)
endif (MSVC)

if (APPLE)
	target_compile_definitions(vfxpro PUBLIC MACOS)

	target_link_libraries(vfxpro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../vfxGraph/macos/lib/libportaudio.a)
endif (APPLE)

if (MSVC)
	set_property(TARGET vfxpro APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
	set_property(TARGET vfxpro APPEND_STRING PROPERTY LINK_FLAGS /SAFESEH:NO)
endif (MSVC)
