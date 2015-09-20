cmake_minimum_required(VERSION 2.6)

set(HAS_LIBNFD 1 PARENT_SCOPE)
message("setting HAS_LIBNFD")

# --- dependencies ---

#

project(libnfd)

# --- libnfd ---

file(GLOB source RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.c" "*.cpp" "*.h")
list(REMOVE_ITEM source nfd_cocoa.m nfd_gtk.c)

add_library(libnfd ${source})

target_include_directories(libnfd PUBLIC "${PROJECT_SOURCE_DIR}")
