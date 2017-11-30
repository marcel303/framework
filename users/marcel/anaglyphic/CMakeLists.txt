cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 14)

set(HAS_LIBGG 0)
set(HAS_FRAMEWORK 0)
set(HAS_LIBVIDEO 0)

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../../../libgg libgg)
endif (NOT HAS_LIBGG)
if (NOT HAS_FRAMEWORK)
	message("including framework")
	add_subdirectory(../../../framework framework)
endif (NOT HAS_FRAMEWORK)
if (NOT HAS_LIBVIDEO)
	message("including libvideo")
	add_subdirectory(../../../libvideo libvideo)
endif (NOT HAS_LIBVIDEO)

#

project(anaglyphic)

# --- anaglyphic ---

file(GLOB_RECURSE source "*.c" "*.cpp" "*.h")

source_group("sources" FILES ${source})

add_executable(anaglyphic ${source})

target_include_directories(anaglyphic PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(anaglyphic PRIVATE framework)
target_link_libraries(anaglyphic PRIVATE libgg)
target_link_libraries(anaglyphic PRIVATE libvideo)

target_link_libraries(anaglyphic PRIVATE ${FRAMEWORK_LIBRARIES})
target_link_libraries(anaglyphic PRIVATE ${LIBVIDEO_LIBRARIES})

if (MSVC)
	set_property(TARGET anaglyphic APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
