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
if (NOT HAS_AVGRAPH)
	message("including avGraph")
	add_subdirectory(../avGraph avGraph)
endif (NOT HAS_AVGRAPH)
if (NOT HAS_LIBVIDEO)
	message("including libvideo")
	add_subdirectory(../libvideo libvideo)
endif (NOT HAS_LIBVIDEO)
if (NOT WIN32 AND NOT HAS_LIBPS3EYE)
	message("including libps3eye")
	add_subdirectory(../libps3eye libps3eye)
endif (NOT WIN32 AND NOT HAS_LIBPS3EYE)

#

project(vfxGraph)

# --- vfxgraph ---

file(GLOB_RECURSE source "*.c" "*.cpp" "*.h" "*.hpp" "*.mm")

file(GLOB_RECURSE source_vfxnodes "vfxNodes/*.cpp" "vfxNodes/*.h" "vfxNodes/*.mm")
file(GLOB_RECURSE source_macos "macos/*.*")
file(GLOB_RECURSE source_win32 "win32/*.*")
file(GLOB_RECURSE source_xmm "xmm/*.*")
file(GLOB_RECURSE source_rtmidi "RtMidi*.*")
file(GLOB_RECURSE source_json "json.hpp")
file(GLOB_RECURSE source_oxfJpegGlitch "ofxJpegGlitch/*.*")

source_group("vfxGraph" FILES ${source})
source_group("vfxNodes" FILES ${source_vfxnodes})
source_group("3rdparty\\macos" FILES ${source_macos})
source_group("3rdparty\\win32" FILES ${source_win32})
source_group("3rdparty\\xmm" FILES ${source_xmm})
source_group("3rdparty\\rtmidi" FILES ${source_rtmidi})
source_group("3rdparty\\json" FILES ${source_json})
source_group("3rdparty\\ofxJpegGlitch" FILES ${source_oxfJpegGlitch})

if (WIN32)
	list(REMOVE_ITEM source ${source_xmm})
endif (WIN32)

# ---- time for some magic ----

# we create a single source file which includes all of the source files in the vfxNodes directory. this serves two purposes,
# 1) it makes sure the constructors of the vfx node registrations run. it uses a trick where at the end of the source file
#    it includes a function, which gets called by createVfxGraph. this ensures that the source file and generated object
#    are references eventually as the linker traverses references starting at the main(..) module. it's a long standing
#    issue initializers are only invoked when objects are actually referenced. this ensure they do
# 2) it speeds up library compilation speed, as all vfx node sources are compiled at once, avoiding the overhead of
#    compiling them separately

if (APPLE)
	set(headerfile ${CMAKE_CURRENT_SOURCE_DIR}/sources.tmp.mm)
else (APPLE)
	set(headerfile ${CMAKE_CURRENT_SOURCE_DIR}/sources.tmp.cpp)
endif (APPLE)
file(WRITE ${headerfile} "// auto-generated. do not hand-edit\n\n")
foreach(s ${source_vfxnodes})
	file(APPEND ${headerfile} "#include \"${s}\"\n")
endforeach(s)
file(APPEND ${headerfile} "void linkVfxNodes() { }\n")

list(APPEND source ${headerfile})
set_source_files_properties(${source_vfxnodes} PROPERTIES HEADER_FILE_ONLY 1)

# ---- end of magic ----

add_library(vfxgraph ${source})

target_link_libraries(vfxgraph avGraph)
target_link_libraries(vfxgraph framework)
target_link_libraries(vfxgraph framework-ofx)
target_link_libraries(vfxgraph libgg)
target_link_libraries(vfxgraph libosc)
target_link_libraries(vfxgraph libvideo)
if (NOT WIN32)
	#target_link_libraries(vfxgraph libps3eye)
endif (NOT WIN32)

target_compile_definitions(vfxgraph PUBLIC
	ENABLE_KINECT1=0
	ENABLE_TURBOJPEG=1)

if (WIN32)
	target_compile_definitions(vfxgraph PUBLIC ENABLE_SYPHON=0 ENABLE_LEAPMOTION=1 ENABLE_KINECT2=0 ENABLE_DEEPBELIEF=0 ENABLE_PS3EYE=0 ENABLE_WIIMOTE=0 __WINDOWS_MM__)
endif (WIN32)

if (APPLE)
	target_compile_definitions(vfxgraph PUBLIC ENABLE_SYPHON=0 ENABLE_LEAPMOTION=0 ENABLE_KINECT2=0 ENABLE_DEEPBELIEF=0 ENABLE_PS3EYE=0 ENABLE_WIIMOTE=0 __MACOSX_CORE__)
endif (APPLE)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	target_compile_definitions(vfxgraph PUBLIC ENABLE_SYPHON=0 ENABLE_LEAPMOTION=0 ENABLE_KINECT2=0 ENABLE_DEEPBELIEF=0 ENABLE_PS3EYE=1 ENABLE_WIIMOTE=0 __LINUX_ALSA__)
endif ()

target_include_directories(vfxgraph PUBLIC "${PROJECT_SOURCE_DIR}")

if (WIN32)
	target_include_directories(vfxgraph PUBLIC "win32/include")
endif (WIN32)

if (APPLE)
	target_include_directories(vfxgraph PUBLIC "macos/include")
endif (APPLE)

if (WIN32)
	target_link_libraries(vfxgraph PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/Leap.lib)
	target_link_libraries(vfxgraph PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/turbojpeg.lib)

	set(VFXGRAPH_LIBRARIES
		PARENT_SCOPE)

	set(VFXGRAPH_BINARIES
		${CMAKE_CURRENT_SOURCE_DIR}/win32/dll/Leap.dll
		${CMAKE_CURRENT_SOURCE_DIR}/win32/dll/turbojpeg.dll
		PARENT_SCOPE)
endif (WIN32)

if (APPLE)
	find_library(COREIMAGE_LIBRARY CoreImage)
	find_library(COREMIDI_LIBRARY CoreMIDI)
	find_library(AVFOUNDATION_LIBRARY AVFoundation)

	set(VFXGRAPH_LIBRARIES
		${CMAKE_CURRENT_SOURCE_DIR}/macos/lib/libturbojpeg.0.dylib
		${COREIMAGE_LIBRARY}
		${COREMIDI_LIBRARY}
		${AVFOUNDATION_LIBRARY}
		${IOBLUETOOH_LIBRARY}
		PARENT_SCOPE)

	set(VFXGRAPH_BINARIES
		${CMAKE_CURRENT_SOURCE_DIR}/macos/lib/libturbojpeg.0.dylib
		PARENT_SCOPE)
endif (APPLE)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	find_library(TURBOJPEG_LIBRARY turbojpeg)
	find_library(ASOUND_LIBRARY asound) # ALSA sound dependency RtMidi

	set(VFXGRAPH_LIBRARIES
		${TURBOJPEG_LIBRARY}
		${ASOUND_LIBRARY}
		PARENT_SCOPE)

	set(VFXGRAPH_BINARIES
		PARENT_SCOPE)
endif ()

if (MSVC)
	set_property(TARGET vfxgraph APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
	set_property(TARGET vfxgraph APPEND_STRING PROPERTY LINK_FLAGS /SAFESEH:NO)
endif (MSVC)
