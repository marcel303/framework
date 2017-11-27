cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 14)

set(HAS_LIBGG 0)
set(HAS_LIBOSC 0)
set(HAS_FRAMEWORK 0)

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

#

project(mitronix)

# --- mitronix ---

file(GLOB_RECURSE source "*.cpp" "*.h")
file(GLOB source_graph "../../../vfxGraph/graph*")

file(GLOB source_audio
	"../../../audioGraph/audio*.*"
	"../../../audioGraph/soundmix*.*"
	"../../../audioGraph/osc4d*.*")
file(GLOB_RECURSE source_audioNodes
	"../../../audioGraph/audioNodes/*.*")
file(GLOB_RECURSE source_objects
	"../../../audioGraph/objects/*.*")
file(GLOB_RECURSE source_delaunay
	"../../../audioGraph/delaunay/*.*")

file(GLOB_RECURSE source_ui
	"../../../libparticle/ui.cpp"
	"../../../libparticle/ui.h"
	"../../../libparticle/textfield.cpp"
	"../../../libparticle/textfield.h"
	"../../../libparticle/colorwheel.cpp"
	"../../../libparticle/colorwheel.h"
	"../../../libparticle/particle.cpp"
	"../../../libparticle/particle.h")

list(APPEND source ${source_graph})
list(APPEND source ${source_audio})
list(APPEND source ${source_audioNodes})
list(APPEND source ${source_objects})
list(APPEND source ${source_delaunay})
list(APPEND source ${source_ui})

source_group("mitronix" FILES ${source})
source_group("audioNodes" FILES ${source_audioNodes})
source_group("objects" FILES ${source_objects})
source_group("tests" FILES ${source_tests})
source_group("ui" FILES ${source_ui})
source_group("3rdparty\\delaunay" FILES ${source_delaunay})

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

target_compile_definitions(mitronix PRIVATE NO_SDL_GLEXT AUDIO_UPDATE_SIZE=64)

target_compile_definitions(mitronix PRIVATE ENABLE_KINECT1=0 ENABLE_KINECT2=0 ENABLE_TURBOJPEG=0 ENABLE_DEEPBELIEF=0 ENABLE_AVCODEC=0 BINAURAL_USE_SSE=1)

if (WIN32)
	target_compile_definitions(mitronix PRIVATE HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX)

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
target_include_directories(mitronix PRIVATE "objects")

if (APPLE)
	target_include_directories(mitronix PRIVATE "../../../audioGraph")
	target_include_directories(mitronix PRIVATE "../../../audioGraph/objects")
	target_include_directories(mitronix PRIVATE "../../../vfxGraph")
	target_include_directories(mitronix PRIVATE "../../../vfxGraph/macos/include")
endif (APPLE)

if (WIN32)
	target_include_directories(mitronix PRIVATE "../../../audioGraph")
	target_include_directories(mitronix PRIVATE "../../../audioGraph/objects")
	target_include_directories(mitronix PRIVATE "../../../vfxGraph")
	target_include_directories(mitronix PRIVATE "../../../vfxGraph/win32/include")
endif (WIN32)

target_link_libraries(mitronix PRIVATE framework)
target_link_libraries(mitronix PRIVATE libosc)
target_link_libraries(mitronix PRIVATE libgg)

target_link_libraries(mitronix PRIVATE ${FRAMEWORK_LIBRARIES})

if (MSVC)
	target_link_libraries(mitronix PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../vfxGraph/win32/lib/portaudio_x86.lib)
endif (MSVC)

if (APPLE)
	target_compile_definitions(mitronix PUBLIC MACOS)

	target_link_libraries(mitronix PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../vfxGraph/macos/lib/libportaudio.a)
endif (APPLE)

if (MSVC)
	set_property(TARGET mitronix APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
	set_property(TARGET mitronix APPEND_STRING PROPERTY LINK_FLAGS /SAFESEH:NO)
endif (MSVC)
