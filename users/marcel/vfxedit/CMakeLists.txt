cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 11)

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

project(vfxedit)

# --- vfxedit ---

file(GLOB_RECURSE source "*.cpp" "*.h")

file(GLOB_RECURSE source_midifile "midifile/*.*")
source_group("midifile" FILES ${source_midifile})

add_executable(vfxedit ${source})

target_compile_definitions(vfxedit PRIVATE NO_SDL_GLEXT HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX)

target_include_directories(vfxedit PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(vfxedit PRIVATE framework)
target_link_libraries(vfxedit PRIVATE libosc)
target_link_libraries(vfxedit PRIVATE libgg)

target_link_libraries(vfxedit PRIVATE ${FRAMEWORK_LIBRARIES})

if (APPLE)
	target_compile_definitions(vfxedit PUBLIC MACOS)
endif (APPLE)

if (MSVC)
	set_property(TARGET vfxedit APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
