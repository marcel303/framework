cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 14)

set(HAS_LIBGG 0)
set(HAS_LIBOSC 0)
set(HAS_FRAMEWORK 0)
set(HAS_FRAMEWORK_OFX 0)
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
if (NOT HAS_FRAMEWORK_OFX)
	message("including framework-ofx")
	add_subdirectory(../framework-ofx framework-ofx)
endif (NOT HAS_FRAMEWORK_OFX)
if (NOT HAS_LIBVIDEO)
	message("including libvideo")
	add_subdirectory(../libvideo libvideo)
endif (NOT HAS_LIBVIDEO)
if (NOT HAS_VFXGRAPH)
	message("including vfxgraph")
	add_subdirectory(../vfxGraph vfxgraph)
endif (NOT HAS_VFXGRAPH)

#

project(framework-tests)

# --- tests ---

file(GLOB_RECURSE source "*.c" "*.cpp" "*.h" "*.hpp" "*.mm")

file(GLOB_RECURSE source_datgui "ofxDatGui/*.*")
file(GLOB_RECURSE source_nanovg "nanovg/*.*")

source_group("sources" FILES ${source})
source_group("3rdparty\\ofxDatGui" FILES ${source_datgui})
source_group("3rdparty\\nanovg" FILES ${source_nanovg})

if (WIN32)
	list(REMOVE_ITEM source ${source_xmm})
endif (WIN32)

add_executable(tests ${source})

set_target_properties(tests PROPERTIES LINK_FLAGS "-Wl,-all_load")

if (WIN32)
	# set the current working directory
	
	set_target_properties(tests PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data")
	
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
	file(COPY ${LIBVIDEO_BINARIES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	
	file(COPY ${FRAMEWORK_BINARIES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${VFXGRAPH_BINARIES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${LIBVIDEO_BINARIES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
endif (APPLE)

target_include_directories(tests PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(tests PRIVATE framework)
target_link_libraries(tests PRIVATE framework-ofx)
target_link_libraries(tests PRIVATE libosc)
target_link_libraries(tests PRIVATE libgg)
target_link_libraries(tests PRIVATE libvideo)
target_link_libraries(tests PRIVATE vfxgraph)

target_link_libraries(tests PRIVATE ${FRAMEWORK_LIBRARIES})
target_link_libraries(tests PRIVATE ${LIBVIDEO_LIBRARIES})
target_link_libraries(tests PRIVATE ${VFXGRAPH_LIBRARIES})

if (MSVC)
	set_property(TARGET tests APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
	set_property(TARGET tests APPEND_STRING PROPERTY LINK_FLAGS /SAFESEH:NO)
endif (MSVC)
