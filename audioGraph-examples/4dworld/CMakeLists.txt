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

project(4dworld)

# --- 4dworld ---

file(GLOB_RECURSE source "*.cpp" "*.h")
file(GLOB source_graph "../vfxGraph/graph*")

file(GLOB source_tests "test*.*")

file(GLOB_RECURSE source_ui
	"../libparticle/ui.cpp"
	"../libparticle/ui.h"
	"../libparticle/textfield.cpp"
	"../libparticle/textfield.h"
	"../libparticle/colorwheel.cpp"
	"../libparticle/colorwheel.h"
	"../libparticle/particle.cpp"
	"../libparticle/particle.h")

list(APPEND source ${source_graph})
list(APPEND source ${source_ui})

source_group("4DWorld" FILES ${source})
source_group("tests" FILES ${source_tests})
source_group("ui" FILES ${source_ui})

add_executable(4dworld ${source})

set_target_properties(4dworld PROPERTIES LINK_FLAGS "-Wl,-all_load")

target_compile_definitions(4dworld PRIVATE
	NO_SDL_GLEXT)

target_link_libraries(4dworld PRIVATE libgg)
target_link_libraries(4dworld PRIVATE libosc)
target_link_libraries(4dworld PRIVATE framework)
target_link_libraries(4dworld PRIVATE audioGraph)

target_link_libraries(4dworld PRIVATE ${FRAMEWORK_LIBRARIES})
target_link_libraries(4dworld PRIVATE ${AUDIOGRAPH_LIBRARIES})

target_include_directories(4dworld PRIVATE "${PROJECT_SOURCE_DIR}")
target_include_directories(4dworld PRIVATE "${PROJECT_SOURCE_DIR}/../../vfxGraph")

if (WIN32)
	target_compile_definitions(4dworld PRIVATE HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX)

	# set the current working directory
	
	set_target_properties(4dworld PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data")
	
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

if (MSVC)
	set_property(TARGET 4dworld APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
	set_property(TARGET 4dworld APPEND_STRING PROPERTY LINK_FLAGS /SAFESEH:NO)
endif (MSVC)
