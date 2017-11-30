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

project(planetvis2)

# --- planetvis2 ---

file(GLOB_RECURSE source "*.c" "*.cpp" "*.h")

source_group("sources" FILES ${source})

add_executable(planetvis2 ${source})

target_include_directories(planetvis2 PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(planetvis2 PRIVATE framework)
target_link_libraries(planetvis2 PRIVATE libgg)

target_link_libraries(planetvis2 PRIVATE ${FRAMEWORK_LIBRARIES})

if (MSVC)
	set_property(TARGET planetvis2 APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
