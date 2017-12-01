cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 14)

set(HAS_AUDIOGRAPH 1 PARENT_SCOPE)
message("setting HAS_AUDIOGRAPH")

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

#

project(audioGraph)

# --- audioGraph ---

file(GLOB_RECURSE source "*.cpp" "*.h")
file(GLOB source_graph "../vfxGraph/graph*")

file(GLOB_RECURSE source_audioNodes "audioNodes/*.*")
file(GLOB_RECURSE source_objects "objects/*.*" ${source_graph})
file(GLOB_RECURSE source_delaunay "delaunay/*.*")

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

source_group("audioGraph" FILES ${source})
source_group("audioNodes" FILES ${source_audioNodes})
source_group("objects" FILES ${source_objects})
source_group("tests" FILES ${source_tests})
source_group("ui" FILES ${source_ui})
source_group("3rdparty\\delaunay" FILES ${source_delaunay})

add_library(audioGraph ${source})

target_link_libraries(audioGraph PRIVATE framework)
target_link_libraries(audioGraph PRIVATE libosc)
target_link_libraries(audioGraph PRIVATE libgg)

target_link_libraries(audioGraph PRIVATE ${FRAMEWORK_LIBRARIES})

target_compile_definitions(audioGraph PUBLIC BINAURAL_USE_SSE=1)

target_include_directories(audioGraph PUBLIC "${PROJECT_SOURCE_DIR}")
target_include_directories(audioGraph PUBLIC "objects")
target_include_directories(audioGraph PUBLIC "../vfxGraph")

if (APPLE)
	target_include_directories(audioGraph PUBLIC "../vfxGraph/macos/include")
endif (APPLE)

if (WIN32)
	target_include_directories(audioGraph PUBLIC "../vfxGraph/win32/include")
endif (WIN32)

if (WIN32)
	set(AUDIOGRAPH_LIBRARIES
		"${CMAKE_CURRENT_SOURCE_DIR}/../vfxGraph/win32/lib/portaudio_x86.lib"
		PARENT_SCOPE)

	set(AUDIOGRAPH_BINARIES
		${CMAKE_CURRENT_SOURCE_DIR}/../vfxGraph/win32/dll/portaudio_x86.dll
		PARENT_SCOPE)
endif (WIN32)

if (APPLE)
	set(AUDIOGRAPH_LIBRARIES
		${CMAKE_CURRENT_SOURCE_DIR}/../vfxGraph/macos/lib/libportaudio.a
		PARENT_SCOPE)

	set(AUDIOGRAPH_BINARIES
		PARENT_SCOPE)
endif (APPLE)

if (MSVC)
	set_property(TARGET audioGraph APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
	set_property(TARGET audioGraph APPEND_STRING PROPERTY LINK_FLAGS /SAFESEH:NO)
endif (MSVC)
