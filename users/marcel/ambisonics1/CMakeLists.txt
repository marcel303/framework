cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 11)

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

project(ambisonics)

# --- ambisonics ---

file(GLOB source "*.cpp" "*.h")

source_group("sources" FILES ${source})

add_executable(ambisonics ${source})

target_include_directories(ambisonics PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(ambisonics PRIVATE framework)
target_link_libraries(ambisonics PRIVATE libgg)

target_link_libraries(ambisonics PRIVATE ${FRAMEWORK_LIBRARIES})
