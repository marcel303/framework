cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 14)

set(HAS_LIBGG 0)
set(HAS_FRAMEWORK 0)

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../../../libgg libgg)
endif (NOT HAS_LIBGG)
if (NOT HAS_FRAMEWORK)
	message("including framework")
	add_subdirectory(../../../framework framework)
endif (NOT HAS_FRAMEWORK)

#

project(magiclines)

# --- magiclines ---

file(GLOB_RECURSE source "*.cpp" "*.h")

source_group("sources" FILES ${source})

add_executable(magiclines ${source})

target_include_directories(magiclines PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(magiclines PRIVATE framework)
target_link_libraries(magiclines PRIVATE libgg)

target_link_libraries(magiclines PRIVATE ${FRAMEWORK_LIBRARIES})

if (MSVC)
	set_property(TARGET magiclines APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
