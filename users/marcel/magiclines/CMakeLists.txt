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
	add_subdirectory(../framework framework)
endif (NOT HAS_FRAMEWORK)

#

project(anaglyphic)

# --- anaglyphic ---

file(GLOB_RECURSE source "*.cpp" "*.h")

add_executable(anaglyphic ${source})

target_compile_definitions(anaglyphic PRIVATE NO_SDL_GLEXT HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX)

target_include_directories(anaglyphic PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(anaglyphic PRIVATE framework)
target_link_libraries(anaglyphic PRIVATE libgg)

target_link_libraries(anaglyphic PRIVATE ${FRAMEWORK_LIBRARIES})

if (MSVC)
	set_property(TARGET anaglyphic APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
