cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 14)

set(HAS_LIBGG 0)
set(HAS_LIBOSC 0)
set(HAS_FRAMEWORK 0)
set(HAS_AUDIOGRAPH 0)

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
if (NOT HAS_AUDIOGRAPH)
	message("including audioGraph")
	add_subdirectory(../audioGraph audioGraph)
endif (NOT HAS_AUDIOGRAPH)

#

project(audioGraph-examples)

# --- examples ---

file(GLOB_RECURSE source "*.c" "*.cpp" "*.h" "*.hpp" "*.mm")

source_group("" FILES ${source})

add_executable(000-minimal 000-minimal.cpp)
add_executable(010-fileselect 010-fileselect.cpp)
add_executable(020-audiogrid 020-audiogrid.cpp)
add_executable(040-gameoflife 040-gameoflife.cpp)
add_executable(300-binauralizer 300-binauralizer.cpp)
add_executable(310-delaunay 310-delaunay.cpp)
add_executable(320-audioVoices 320-audioVoiceManager.cpp)
add_executable(330-reflections 330-reflections.cpp)
add_executable(340-soundVolumes 340-soundVolumes.cpp)
add_executable(360-speeders 360-speeders.cpp)
add_executable(400-editor 400-editor.cpp)
add_executable(420-realtime-editing 420-realtime-editing.cpp)
add_executable(440-realtime-multiwindow 440-realtime-multiwindow.cpp)
add_executable(480-editor-customdraw 480-editor-customdraw.cpp)
add_executable(500-consoleapp 500-consoleapp.cpp)
add_executable(700-4dworld 700-4dworld.cpp)
add_executable(710-go 710-go.cpp)

set(LIBRARIES PRIVATE
	libgg
	libosc
	framework
	audioGraph
	${FRAMEWORK_LIBRARIES}
	${AUDIOGRAPH_LIBRARIES})

target_link_libraries(000-minimal PRIVATE ${LIBRARIES})
target_link_libraries(010-fileselect PRIVATE ${LIBRARIES})
target_link_libraries(020-audiogrid PRIVATE ${LIBRARIES})
target_link_libraries(040-gameoflife PRIVATE ${LIBRARIES})
target_link_libraries(300-binauralizer PRIVATE ${LIBRARIES})
target_link_libraries(310-delaunay PRIVATE ${LIBRARIES})
target_link_libraries(320-audioVoices PRIVATE ${LIBRARIES})
target_link_libraries(330-reflections PRIVATE ${LIBRARIES})
target_link_libraries(340-soundVolumes PRIVATE ${LIBRARIES})
target_link_libraries(360-speeders PRIVATE ${LIBRARIES})
target_link_libraries(400-editor PRIVATE ${LIBRARIES})
target_link_libraries(420-realtime-editing PRIVATE ${LIBRARIES})
target_link_libraries(440-realtime-multiwindow PRIVATE ${LIBRARIES})
target_link_libraries(480-editor-customdraw PRIVATE ${LIBRARIES})
target_link_libraries(500-consoleapp PRIVATE ${LIBRARIES})
target_link_libraries(700-4dworld PRIVATE ${LIBRARIES})
target_link_libraries(710-go PRIVATE ${LIBRARIES})

if (WIN32)
	# set the current working directory
	
	set_target_properties(300-binauralizer PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data")
	set_target_properties(310-delaunay PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data")
	set_target_properties(320-audioVoices PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data")
	set_target_properties(700-4dworld PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data")
	set_target_properties(710-go PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data")
	
	# copy DLL files to the Debug and Release folders
	
	file(COPY ${FRAMEWORK_BINARIES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${AUDIOGRAPH_BINARIES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	
	file(COPY ${FRAMEWORK_BINARIES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${AUDIOGRAPH_BINARIES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
endif (WIN32)

if (APPLE)
	file(COPY ${FRAMEWORK_BINARIES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${AUDIOGRAPH_BINARIES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)

	file(COPY ${FRAMEWORK_BINARIES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${AUDIOGRAPH_BINARIES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
endif (APPLE)

if (MSVC)
	set_property(TARGET 700-4dworld APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
	set_property(TARGET 700-4dworld APPEND_STRING PROPERTY LINK_FLAGS /SAFESEH:NO)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT 700-4dworld)
endif (MSVC)
