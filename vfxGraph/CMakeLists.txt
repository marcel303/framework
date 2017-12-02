cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 14)

set(HAS_VFXGRAPH 1 PARENT_SCOPE)
message("setting HAS_VFXGRAPH")

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

#

project(vfxGraph)

# --- vfxgraph ---

file(GLOB_RECURSE source "*.c" "*.cpp" "*.h" "*.hpp" "*.mm")

file(GLOB_RECURSE source_vfxnodes "vfxNodes/*.*")
file(GLOB_RECURSE source_macos "macos/*.*")
file(GLOB_RECURSE source_win32 "win32/*.*")
file(GLOB_RECURSE source_xmm "xmm/*.*")
file(GLOB_RECURSE source_rtmidi "RtMidi*.*")
file(GLOB_RECURSE source_json "json.hpp")
file(GLOB_RECURSE source_oxfJpegGlitch "ofxJpegGlitch/*.*")

file(GLOB_RECURSE source_ui
	"../libparticle/ui.cpp"
	"../libparticle/ui.h"
	"../libparticle/textfield.cpp"
	"../libparticle/textfield.h"
	"../libparticle/colorwheel.cpp"
	"../libparticle/colorwheel.h"
	"../libparticle/particle.cpp"
	"../libparticle/particle.h")

list(APPEND source ${source_ui})

source_group("vfxGraph" FILES ${source})
source_group("vfxNodes" FILES ${source_vfxnodes})
source_group("ui" FILES ${source_ui})
source_group("3rdparty\\macos" FILES ${source_macos})
source_group("3rdparty\\win32" FILES ${source_win32})
source_group("3rdparty\\xmm" FILES ${source_xmm})
source_group("3rdparty\\rtmidi" FILES ${source_rtmidi})
source_group("3rdparty\\json" FILES ${source_json})
source_group("3rdparty\\ofxJpegGlitch" FILES ${source_oxfJpegGlitch})

if (WIN32)
	list(REMOVE_ITEM source ${source_xmm})
endif (WIN32)

add_library(vfxgraph ${source})

target_link_libraries(vfxgraph framework)
target_link_libraries(vfxgraph framework-ofx)
target_link_libraries(vfxgraph libgg)
target_link_libraries(vfxgraph libosc)
target_link_libraries(vfxgraph libvideo)

target_compile_definitions(vfxgraph PUBLIC
	ENABLE_KINECT1=0
	ENABLE_TURBOJPEG=1
	ENABLE_LEAPMOTION=1)

if (WIN32)
	target_compile_definitions(vfxgraph PUBLIC ENABLE_KINECT2=0 ENABLE_DEEPBELIEF=0 __WINDOWS_MM__)
endif (WIN32)

if (APPLE)
	target_compile_definitions(vfxgraph PUBLIC ENABLE_KINECT2=1 ENABLE_DEEPBELIEF=1 __MACOSX_CORE__)
endif (APPLE)

target_include_directories(vfxgraph PUBLIC "${PROJECT_SOURCE_DIR}")

if (WIN32)
	target_include_directories(vfxgraph PUBLIC "win32/include")
endif (WIN32)

if (APPLE)
	target_include_directories(vfxgraph PUBLIC "macos/include")
	target_include_directories(vfxgraph PRIVATE "../libfreenect2/macos/include")
endif (APPLE)

if (WIN32)
	target_link_libraries(vfxgraph PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/Leap.lib)
	target_link_libraries(vfxgraph PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/portaudio_x86.lib)
	target_link_libraries(vfxgraph PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/turbojpeg.lib)

	set(VFXGRAPH_LIBRARIES
		PARENT_SCOPE)

	set(VFXGRAPH_BINARIES
		${CMAKE_CURRENT_SOURCE_DIR}/win32/dll/Leap.dll
		${CMAKE_CURRENT_SOURCE_DIR}/win32/dll/portaudio_x86.dll
		${CMAKE_CURRENT_SOURCE_DIR}/win32/dll/turbojpeg.dll
		PARENT_SCOPE)
endif (WIN32)

if (APPLE)
	target_link_libraries(vfxgraph ${CMAKE_CURRENT_SOURCE_DIR}/macos/framework/DeepBelief.framework)
	target_link_libraries(vfxgraph ${CMAKE_CURRENT_SOURCE_DIR}/macos/framework/Syphon.framework)

	find_library(COREIMAGE_LIBRARY CoreImage)
	find_library(COREMIDI_LIBRARY CoreMIDI)
	find_library(AVFOUNDATION_LIBRARY AVFoundation)

	set(VFXGRAPH_LIBRARIES
		${CMAKE_CURRENT_SOURCE_DIR}/../libfreenect2/macos/lib/libfreenect2.0.2.0.dylib
		${CMAKE_CURRENT_SOURCE_DIR}/macos/lib/libLeap.dylib
		${CMAKE_CURRENT_SOURCE_DIR}/macos/lib/libportaudio.a
		${CMAKE_CURRENT_SOURCE_DIR}/macos/lib/libturbojpeg.0.dylib
		${CMAKE_CURRENT_SOURCE_DIR}/macos/framework/DeepBelief.framework
		${CMAKE_CURRENT_SOURCE_DIR}/macos/framework/Syphon.framework
		${COREIMAGE_LIBRARY}
		${COREMIDI_LIBRARY}
		${AVFOUNDATION_LIBRARY}
		PARENT_SCOPE)

	set(VFXGRAPH_BINARIES
		${CMAKE_CURRENT_SOURCE_DIR}/macos/framework/Deepbelief.framework
		${CMAKE_CURRENT_SOURCE_DIR}/macos/framework/Syphon.framework
		${CMAKE_CURRENT_SOURCE_DIR}/macos/lib/libLeap.dylib
		${CMAKE_CURRENT_SOURCE_DIR}/macos/lib/libturbojpeg.0.dylib
		PARENT_SCOPE)
endif (APPLE)

if (MSVC)
	set_property(TARGET vfxgraph APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
	set_property(TARGET vfxgraph APPEND_STRING PROPERTY LINK_FLAGS /SAFESEH:NO)
endif (MSVC)
