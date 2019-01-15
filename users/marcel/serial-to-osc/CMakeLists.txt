cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.11")

set(HAS_LIBGG 0)
set(HAS_LIBOSC 0)

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../../../libgg libgg)
endif (NOT HAS_LIBGG)
if (NOT HAS_LIBOSC)
	message("including libosc")
	add_subdirectory(../../../libosc libosc)
endif (NOT HAS_LIBOSC)

#

project(serial-to-osc)

# --- serial-to-osc ---

file(GLOB source "*.c" "*.cpp" "*.h" "*.hpp" "*.mm")

source_group("sources" FILES ${source})

add_executable(serial-to-osc MACOSX_BUNDLE ${source} ${resources})

target_include_directories(serial-to-osc PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(serial-to-osc PRIVATE libosc)
target_link_libraries(serial-to-osc PRIVATE libgg)
