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

source_group("" FILES ${source})

add_executable(000-minimal "000-minimal.cpp")
add_executable(010-multiple-instances "010-multiple-instances.cpp")
add_executable(012-happy-newyear "012-happy-newyear.cpp")
add_executable(400-editor "400-editor.cpp")
add_executable(410-readonly-editor "410-readonly-editor.cpp")
add_executable(420-realtime-editing "420-realtime-editing.cpp")
add_executable(440-realtime-multiwindow "440-realtime-multiwindow.cpp")
add_executable(900-devgrounds "900-devgrounds.cpp")

set(LIBRARIES PRIVATE
	libgg
	libosc
	framework
	libvideo
	vfxgraph
	${FRAMEWORK_LIBRARIES}
	${LIBVIDEO_LIBRARIES}
	${VFXGRAPH_LIBRARIES})

target_link_libraries(000-minimal PRIVATE ${LIBRARIES})
target_link_libraries(010-multiple-instances PRIVATE ${LIBRARIES})
target_link_libraries(012-happy-newyear PRIVATE ${LIBRARIES})
target_link_libraries(400-editor PRIVATE ${LIBRARIES})
target_link_libraries(410-readonly-editor PRIVATE ${LIBRARIES})
target_link_libraries(420-realtime-editing PRIVATE ${LIBRARIES})
target_link_libraries(440-realtime-multiwindow PRIVATE ${LIBRARIES})
target_link_libraries(900-devgrounds PRIVATE ${LIBRARIES})

if (WIN32)
	# set the current working directory
	
	set_target_properties(000-minimal PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data")
	set_target_properties(010-multiple-instances PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data")
	set_target_properties(012-happy-newyear PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data")
	set_target_properties(400-editor PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data")
	set_target_properties(410-readonly-editor PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data")
	set_target_properties(420-realtime-editing PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data")
	set_target_properties(440-realtime-multiwindow PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data")
	set_target_properties(900-devgrounds PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data")
	
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
	set_property(TARGET 000-minimal APPEND_STRING PROPERTY LINK_FLAGS "/SAFESEH:NO")
	set_property(TARGET 010-multiple-instances APPEND_STRING PROPERTY LINK_FLAGS "/SAFESEH:NO")
	set_property(TARGET 012-happy-newyear APPEND_STRING PROPERTY LINK_FLAGS "/SAFESEH:NO")
	set_property(TARGET 400-editor APPEND_STRING PROPERTY LINK_FLAGS "/SAFESEH:NO")
	set_property(TARGET 410-readonly-editor APPEND_STRING PROPERTY LINK_FLAGS "/SAFESEH:NO")
	set_property(TARGET 420-realtime-editing APPEND_STRING PROPERTY LINK_FLAGS "/SAFESEH:NO")
	set_property(TARGET 440-realtime-multiwindow APPEND_STRING PROPERTY LINK_FLAGS "/SAFESEH:NO")
	set_property(TARGET 900-devgrounds APPEND_STRING PROPERTY LINK_FLAGS "/SAFESEH:NO")
endif (MSVC)
