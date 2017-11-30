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

project(cuber)

# --- cuber ---

file(GLOB source RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cpp" "*.h")
list(REMOVE_ITEM source script.cpp)

source_group("sources" FILES ${source})

add_executable(cuber ${source})

target_include_directories(cuber PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(cuber PRIVATE framework)
target_link_libraries(cuber PRIVATE libgg)

target_link_libraries(cuber PRIVATE ${FRAMEWORK_LIBRARIES})

if (MSVC)
	set_property(TARGET cuber APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)

# --- script ---

set(WINDOWS_EXPORT_ALL_SYMBOLS, 1)

add_library(script SHARED OpenSimplexNoise.cpp OpenSimplexNoise.h script.cpp script.h simplexnoise.cpp simplexnoise.h)

if (APPLE)
	target_include_directories(script PRIVATE "${PROJECT_SOURCE_DIR}")
	target_include_directories(script PRIVATE "${PROJECT_SOURCE_DIR}/../../../framework/macos/include")
endif (APPLE)

if (MSVC)
	target_include_directories(script PRIVATE "${PROJECT_SOURCE_DIR}")
	target_include_directories(script PRIVATE "${PROJECT_SOURCE_DIR}/../../../framework/win32/include")
endif (MSVC)

target_link_libraries(script PRIVATE libgg)

if (MSVC)
	set_property(TARGET script APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
