cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 14)

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../../libgg libgg)
endif (NOT HAS_LIBGG)
if (NOT HAS_FRAMEWORK)
	message("including framework")
	add_subdirectory(../../framework framework)
endif (NOT HAS_FRAMEWORK)

#

project(framework-tests)

# --- tests ---

add_executable(aniplay aniplay.cpp)
add_executable(example example.cpp)
add_executable(model model.cpp)
add_executable(sounds sounds.cpp)
add_executable(sprites sprites.cpp)
add_executable(spriter spriter.cpp)

target_link_libraries(aniplay PRIVATE ${FRAMEWORK_LIBRARIES})
target_link_libraries(example PRIVATE ${FRAMEWORK_LIBRARIES})
target_link_libraries(model PRIVATE ${FRAMEWORK_LIBRARIES})
target_link_libraries(sounds PRIVATE ${FRAMEWORK_LIBRARIES})
target_link_libraries(sprites PRIVATE ${FRAMEWORK_LIBRARIES})
target_link_libraries(spriter PRIVATE ${FRAMEWORK_LIBRARIES})

if (WIN32)
	# set the current working directory
	
	set_target_properties(4dworld PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data")
	set_target_properties(binauralizer PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data")
	set_target_properties(delaunay PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data")
	set_target_properties(audioVoices PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data")
	
	# copy DLL files to the Debug and Release folders
	
	file(COPY ${FRAMEWORK_BINARIES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${FRAMEWORK_BINARIES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
endif (WIN32)

if (APPLE)
	file(COPY ${FRAMEWORK_BINARIES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${FRAMEWORK_BINARIES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
endif (APPLE)

if (MSVC)
	set_property(TARGET libparticle_test APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
	set_property(TARGET libparticle_test APPEND_STRING PROPERTY LINK_FLAGS /SAFESEH:NO)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT model)
endif (MSVC)
