cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 14)

set(HAS_LIBGG 0)
set(HAS_LIBOSC 0)
set(HAS_FRAMEWORK 0)
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
if (NOT HAS_AUDIOGRAPH)
	message("including audioGraph")
	add_subdirectory(../../../audioGraph audioGraph)
endif (NOT HAS_AUDIOGRAPH)

#

project(withus)

# --- withus ---

file(GLOB source "*.cpp" "*.h")

source_group("sources" FILES ${source})

file(GLOB resources_root
	"${CMAKE_CURRENT_SOURCE_DIR}/data/*.jpg"
	"${CMAKE_CURRENT_SOURCE_DIR}/data/examples/*.ogg"
	"${CMAKE_CURRENT_SOURCE_DIR}/data/*.ttf*"
	"${CMAKE_CURRENT_SOURCE_DIR}/data/*.xml")

set_source_files_properties(${resources_root} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
source_group("resources" FILES ${resources_root})

add_executable(withus MACOSX_BUNDLE ${source} ${resources_root})

target_include_directories(withus PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(withus PRIVATE audioGraph)
target_link_libraries(withus PRIVATE framework)
target_link_libraries(withus PRIVATE libosc)
target_link_libraries(withus PRIVATE libgg)

target_link_libraries(withus PRIVATE ${FRAMEWORK_LIBRARIES})
target_link_libraries(withus PRIVATE ${AUDIOGRAPH_LIBRARIES})
