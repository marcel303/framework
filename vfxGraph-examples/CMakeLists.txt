cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 14)

set(HAS_LIBGG 0)
set(HAS_LIBOSC 0)
set(HAS_FRAMEWORK 0)
set(HAS_LIBVIDEO 0)
set(HAS_VFXGRAPH 0)

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../libgg libgg)
endif (NOT HAS_LIBGG)
if (NOT HAS_LIBOSC)
	message("including libosc")
	add_subdirectory(../libosc libosc)
endif (NOT HAS_LIBOSC)
if (NOT HAS_FRAMEWORK)
	message("including framework")
	add_subdirectory(../framework framework)
endif (NOT HAS_FRAMEWORK)
if (NOT HAS_LIBVIDEO)
	message("including libvideo")
	add_subdirectory(../libvideo libvideo)
endif (NOT HAS_LIBVIDEO)
if (NOT HAS_VFXGRAPH)
	message("including vfxgraph")
	add_subdirectory(../vfxGraph vfxgraph)
endif (NOT HAS_VFXGRAPH)

#

project(vfxGraph-examples)

# --- examples ---

file(GLOB_RECURSE source "*.c" "*.cpp" "*.h" "*.hpp" "*.mm")

source_group("sources" FILES ${source})

add_executable(examples ${source})

if (APPLE)
	set_target_properties(examples PROPERTIES LINK_FLAGS "-Wl,-all_load")
endif (APPLE)

set(LIBRARIES PRIVATE
	libgg
	libosc
	framework
	libvideo
	vfxgraph
	${FRAMEWORK_LIBRARIES}
	${LIBVIDEO_LIBRARIES}
	${VFXGRAPH_LIBRARIES})

target_link_libraries(examples PRIVATE ${LIBRARIES})

target_include_directories(examples PRIVATE "${PROJECT_SOURCE_DIR}")

if (WIN32)
	# set the current working directory
	
	set_target_properties(examples PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data")
	
	# copy DLL files to the Debug and Release folders

	file(COPY ${FRAMEWORK_BINARIES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${VFXGRAPH_BINARIES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${LIBVIDEO_BINARIES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	
	file(COPY ${FRAMEWORK_BINARIES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${VFXGRAPH_BINARIES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${LIBVIDEO_BINARIES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
endif (WIN32)

if (APPLE)
	# copy dylib and framework files to the Debug and Release folders

	file(COPY ${FRAMEWORK_BINARIES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${VFXGRAPH_BINARIES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)

	file(COPY ${FRAMEWORK_BINARIES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${VFXGRAPH_BINARIES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
endif (APPLE)

if (MSVC)
	set_property(TARGET examples APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
	set_property(TARGET examples APPEND_STRING PROPERTY LINK_FLAGS "/SAFESEH:NO /WHOLEARCHIVE")
endif (MSVC)
