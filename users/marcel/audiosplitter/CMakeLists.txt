cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 11)

set(HAS_LIBGG 0)
set(HAS_FRAMEWORK 0)
set(HAS_AUDIOGRAPH 0)

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../../../libgg libgg)
endif (NOT HAS_LIBGG)
if (NOT HAS_FRAMEWORK)
	message("including framework")
	add_subdirectory(../../../framework framework)
endif (NOT HAS_FRAMEWORK)
if (NOT HAS_AUDIOGRAPH)
	message("including audioGraph")
	add_subdirectory(../../../audioGraph audioGraph)
endif (NOT HAS_AUDIOGRAPH)

#

project(audiosplitter)

# --- audiosplitter ---

file(GLOB_RECURSE source "*.cpp" "*.h")

source_group("sources" FILES ${source})

add_executable(audiosplitter ${source})

target_include_directories(audiosplitter PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(audiosplitter PRIVATE audioGraph)
target_link_libraries(audiosplitter PRIVATE framework)
target_link_libraries(audiosplitter PRIVATE libgg)

target_link_libraries(audiosplitter PRIVATE ${FRAMEWORK_LIBRARIES})
