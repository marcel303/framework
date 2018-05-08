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
if (NOT HAS_AVGRAPH)
	message("including avGraph")
	add_subdirectory(../avGraph avGraph)
endif (NOT HAS_AVGRAPH)

#

project(audioGraph)

# --- audioGraph ---

file(GLOB_RECURSE source "*.cpp" "*.h")
file(GLOB_RECURSE source_audioNodes "audioNodes/*.cpp" "audioNodes/*.h")
file(GLOB_RECURSE source_objects "objects/*.*" ${source_graph})
file(GLOB_RECURSE source_delaunay "delaunay/*.*")

file(GLOB source_tests "test*.*")

source_group("audioGraph" FILES ${source})
source_group("audioNodes" FILES ${source_audioNodes})
source_group("objects" FILES ${source_objects})
source_group("tests" FILES ${source_tests})
source_group("3rdparty\\delaunay" FILES ${source_delaunay})

# ---- time for some magic ----

# we create a single source file which includes all of the source files in the audioNodes directory. this serves two purposes,
# 1) it makes sure the constructors of the audio node registrations run. it uses a trick where at the end of the source file
#    it includes a function, which gets called by createAudioGraph. this ensures that the source file and generated object
#    are references eventually as the linker traverses references starting at the main(..) module. it's a long standing
#    issue initializers are only invoked when objects are actually referenced. this ensure they do
# 2) it speeds up library compilation speed, as all audio node sources are compiled at once, avoiding the overhead of
#    compiling them separately

set(headerfile ${CMAKE_CURRENT_SOURCE_DIR}/sources.tmp.cpp)
file(WRITE ${headerfile} "// auto-generated. do not hand-edit\n\n")
foreach(s ${source_audioNodes})
	file(APPEND ${headerfile} "#include \"${s}\"\n")
endforeach(s)
file(APPEND ${headerfile} "void linkAudioNodes() { }\n")

list(APPEND source ${headerfile})
set_source_files_properties(${source_audioNodes} PROPERTIES HEADER_FILE_ONLY 1)

# ---- end of magic ----

add_library(audioGraph ${source})

target_link_libraries(audioGraph PRIVATE avGraph)
target_link_libraries(audioGraph PRIVATE framework)
target_link_libraries(audioGraph PRIVATE libosc)
target_link_libraries(audioGraph PRIVATE libgg)

target_link_libraries(audioGraph PRIVATE ${FRAMEWORK_LIBRARIES})

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
		PARENT_SCOPE)

	set(AUDIOGRAPH_BINARIES
		PARENT_SCOPE)
endif (WIN32)

if (APPLE)
	set(AUDIOGRAPH_LIBRARIES
		PARENT_SCOPE)

	set(AUDIOGRAPH_BINARIES
		PARENT_SCOPE)
endif (APPLE)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux") 
	set(CMAKE_CXX_FLAGS "-mcpu=cortex-a53 -mfpu=neon-fp-armv8 -mfloat-abi=hard -funsafe-math-optimizations")
endif ()

if (MSVC)
	set_property(TARGET audioGraph APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
	set_property(TARGET audioGraph APPEND_STRING PROPERTY LINK_FLAGS /SAFESEH:NO)
endif (MSVC)
