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

add_executable(planetvis2 ${source})

target_compile_definitions(planetvis2 PRIVATE NO_SDL_GLEXT HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX)

target_include_directories(planetvis2 PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(planetvis2 PRIVATE framework)
target_link_libraries(planetvis2 PRIVATE libgg)

target_link_libraries(planetvis2 PRIVATE ${FRAMEWORK_LIBRARIES})

if (APPLE)
	target_compile_definitions(planetvis2 PUBLIC MACOS)
endif (APPLE)

if (MSVC)
	set_property(TARGET planetvis2 APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
