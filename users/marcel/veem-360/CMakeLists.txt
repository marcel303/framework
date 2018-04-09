cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR})

set(HAS_LIBGG 0)
set(HAS_LIBOSC 0)
set(HAS_FRAMEWORK 0)
set(HAS_LIBVIDEO 0)
set(HAS_AVGRAPH 0)
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
if (NOT HAS_AUDIOGRAPH)
	message("including audioGraph")
	add_subdirectory(../../../audioGraph audioGraph)
endif (NOT HAS_AUDIOGRAPH)

#

project(veem-360)

# --- veem-360 ---

file(GLOB source "*.cpp" "*.h")

source_group("sources" FILES ${source})
file(GLOB resources
	"${CMAKE_CURRENT_SOURCE_DIR}/calibri.ttf"
	"${CMAKE_CURRENT_SOURCE_DIR}/01.mp4"
	"${CMAKE_CURRENT_SOURCE_DIR}/sound.wav"
	"${CMAKE_CURRENT_SOURCE_DIR}/360video.vs"
	"${CMAKE_CURRENT_SOURCE_DIR}/360video.ps")

set_source_files_properties(${resources} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

add_executable(veem-360 MACOSX_BUNDLE ${source} ${resources})

target_include_directories(veem-360 PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(veem-360 PRIVATE ${LIBVIDEO_LIBRARIES})

target_link_libraries(veem-360 PRIVATE audioGraph)
target_link_libraries(veem-360 PRIVATE avGraph)
target_link_libraries(veem-360 PRIVATE libvideo)
target_link_libraries(veem-360 PRIVATE framework)
target_link_libraries(veem-360 PRIVATE libosc)
target_link_libraries(veem-360 PRIVATE libgg)

target_link_libraries(veem-360 PRIVATE ${FRAMEWORK_LIBRARIES})


if (APPLE)
	# copy dylib and framework files to the Debug and Release folders

	file(COPY ${FRAMEWORK_BINARIES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)

	file(COPY ${FRAMEWORK_BINARIES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
endif (APPLE)

install(SCRIPT bundle.cmake)
