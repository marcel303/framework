cmake_minimum_required(VERSION 2.6)

set(HAS_LIBNFD 1 PARENT_SCOPE)
message("setting HAS_LIBNFD")

# --- dependencies ---

#

project(libnfd)

# --- libnfd ---

file(GLOB source RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.c" "*.cpp" "*.h" "*.m")
if (MSVC)
	list(REMOVE_ITEM source nfd_cocoa.m nfd_gtk.c)
endif (MSVC)
if (APPLE)
	list(REMOVE_ITEM source nfd_gtk.c nfd_win.cpp)
endif (APPLE)

source_group("sources" FILES ${source})

add_library(libnfd ${source})

target_include_directories(libnfd PUBLIC "${PROJECT_SOURCE_DIR}")
