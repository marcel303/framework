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

project(mitronix)

# --- mitronix ---

file(GLOB_RECURSE source "*.cpp" "*.h")

source_group("mitronix" FILES ${source})

file(GLOB resources_root
	"${CMAKE_CURRENT_SOURCE_DIR}/data/*.jpg"
	"${CMAKE_CURRENT_SOURCE_DIR}/data/*.ttf*"
	"${CMAKE_CURRENT_SOURCE_DIR}/data/*.xml")
file(GLOB resources_cipic147
	"${CMAKE_CURRENT_SOURCE_DIR}/data/cipic.147/*.wav")

set_source_files_properties(${resources_root} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
set_source_files_properties(${resources_cipic147} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/cipic.147")
message(STATUS "CIPIC resources: " ${resources_cipic147})
source_group("resources" FILES ${resources_root} ${resources_cipic147})

add_executable(mitronix MACOSX_BUNDLE ${source} ${resources_root} ${resources_cipic147})

set_target_properties(mitronix PROPERTIES LINK_FLAGS "-Wl,-all_load")

target_compile_definitions(mitronix PUBLIC AUDIO_UPDATE_SIZE=64)

if (WIN32)
	# set the current working directory
	
	set_target_properties(mitronix PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data")
	
	# copy DLL files to the Debug and Release folders
	
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/dll/FreeImage.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/dll/glew32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/dll/OpenAL32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/dll/SDL2.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/dll/XInput9_1_0.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../avpaint/win32/dll/portaudio_x86.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/dll/FreeImage.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/dll/glew32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/dll/OpenAL32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/dll/SDL2.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/dll/XInput9_1_0.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../avpaint/win32/dll/portaudio_x86.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
endif (WIN32)

target_include_directories(mitronix PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(mitronix PRIVATE audioGraph)
target_link_libraries(mitronix PRIVATE framework)
target_link_libraries(mitronix PRIVATE libosc)
target_link_libraries(mitronix PRIVATE libgg)

target_link_libraries(mitronix PRIVATE ${FRAMEWORK_LIBRARIES})
target_link_libraries(mitronix PRIVATE ${AUDIOGRAPH_LIBRARIES})

if (MSVC)
	set_property(TARGET mitronix APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
	set_property(TARGET mitronix APPEND_STRING PROPERTY LINK_FLAGS /SAFESEH:NO)
endif (MSVC)
