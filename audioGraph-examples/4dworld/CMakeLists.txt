cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 14)

set(HAS_LIBGG 0)
set(HAS_LIBOSC 0)
set(HAS_FRAMEWORK 0)
set(HAS_AUDIOGRAPH 0)

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../../libgg libgg)
endif (NOT HAS_LIBGG)
if (NOT HAS_LIBOSC)
	message("including libosc")
	add_subdirectory(../../libosc libosc)
endif (NOT HAS_LIBOSC)
if (NOT HAS_FRAMEWORK)
	message("including framework")
	add_subdirectory(../../framework framework)
endif (NOT HAS_FRAMEWORK)
if (NOT HAS_AUDIOGRAPH)
	message("including audioGraph")
	add_subdirectory(../../audioGraph audioGraph)
endif (NOT HAS_AUDIOGRAPH)

#

project(audioGraph-examples)

# --- examples ---

file(GLOB_RECURSE source_all "*.cpp" "*.h")
file(GLOB_RECURSE source_4dworld "4dworld.cpp")
file(GLOB_RECURSE source_binauralizer "binauralizer.cpp")
file(GLOB_RECURSE source_delaunay "delaunay.cpp")
file(GLOB_RECURSE source_audioVoices "audioVoiceManager.cpp")

source_group("sources" FILES ${source_all})

add_executable(4dworld ${source_4dworld})
add_executable(binauralizer ${source_binauralizer})
add_executable(delaunay ${source_delaunay})
add_executable(audioVoices ${source_audioVoices})

set_target_properties(4dworld PROPERTIES LINK_FLAGS "-Wl,-all_load")
set_target_properties(binauralizer PROPERTIES LINK_FLAGS "-Wl,-all_load")
set_target_properties(delaunay PROPERTIES LINK_FLAGS "-Wl,-all_load")
set_target_properties(audioVoices PROPERTIES LINK_FLAGS "-Wl,-all_load")

set(LIBRARIES PRIVATE
	libgg
	libosc
	framework
	audioGraph
	${FRAMEWORK_LIBRARIES}
	${AUDIOGRAPH_LIBRARIES})

target_link_libraries(4dworld PRIVATE ${LIBRARIES})
target_link_libraries(binauralizer PRIVATE ${LIBRARIES})
target_link_libraries(delaunay PRIVATE ${LIBRARIES})
target_link_libraries(audioVoices PRIVATE ${LIBRARIES})

if (WIN32)
	# set the current working directory
	
	set_target_properties(4dworld PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data")
	set_target_properties(binauralizer PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data")
	set_target_properties(delaunay PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data")
	set_target_properties(audioVoices PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data")
	
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
	set_property(TARGET 4dworld APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
	set_property(TARGET 4dworld APPEND_STRING PROPERTY LINK_FLAGS /SAFESEH:NO)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT 4dworld)
endif (MSVC)
