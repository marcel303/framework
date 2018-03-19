cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR})

set(HAS_LIBGG 0)
set(HAS_LIBOSC 0)
set(HAS_FRAMEWORK 0)
set(HAS_AVGRAPH 0)
set(HAS_VFXGRAPH 0)
set(HAS_AUDIOGRAPH 0)

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
if (NOT HAS_AVGRAPH)
	message("including avGraph")
	add_subdirectory(../../../avGraph avGraph)
endif (NOT HAS_AVGRAPH)
if (NOT HAS_VFXGRAPH)
	message("including vfxGraph")
	add_subdirectory(../../../vfxgraph vfxgraph)
endif (NOT HAS_VFXGRAPH)
if (NOT HAS_AUDIOGRAPH)
	message("including audioGraph")
	add_subdirectory(../../../audioGraph audioGraph)
endif (NOT HAS_AUDIOGRAPH)

#

project(veem-sound-satellites)

# --- veem-sound-satellites ---

file(GLOB source "*.cpp" "*.h")

source_group("sources" FILES ${source})

add_executable(veem-sound-satellites MACOSX_BUNDLE ${source})

target_include_directories(veem-sound-satellites PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(veem-sound-satellites PRIVATE ${LIBVIDEO_LIBRARIES})
target_link_libraries(veem-sound-satellites PRIVATE ${VFXGRAPH_LIBRARIES})

target_link_libraries(veem-sound-satellites PRIVATE audioGraph)
target_link_libraries(veem-sound-satellites PRIVATE vfxgraph)
target_link_libraries(veem-sound-satellites PRIVATE avGraph)
target_link_libraries(veem-sound-satellites PRIVATE framework)
target_link_libraries(veem-sound-satellites PRIVATE libosc)
target_link_libraries(veem-sound-satellites PRIVATE libgg)

target_link_libraries(veem-sound-satellites PRIVATE ${FRAMEWORK_LIBRARIES})

install(SCRIPT bundle.cmake)
