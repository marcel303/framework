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

project(grooop-cocompose)

# --- grooop-cocompose ---

file(GLOB_RECURSE source "*.cpp" "*.h")

add_executable(grooop-cocompose ${source})

target_include_directories(grooop-cocompose PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(grooop-cocompose PRIVATE framework)
target_link_libraries(grooop-cocompose PRIVATE libgg)

target_link_libraries(grooop-cocompose PRIVATE ${FRAMEWORK_LIBRARIES})

if (MSVC)
	set_property(TARGET grooop-cocompose APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
