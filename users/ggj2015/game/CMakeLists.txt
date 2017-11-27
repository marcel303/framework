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

project(game)

# --- game ---

file(GLOB source "*.cpp" "*.h")

add_executable(game ${source})

target_compile_definitions(game PRIVATE NO_SDL_GLEXT HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)

target_include_directories(game PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(game PRIVATE framework)
target_link_libraries(game PRIVATE libgg)

target_link_libraries(colorpart PRIVATE ${FRAMEWORK_LIBRARIES})

if (APPLE)
	target_compile_definitions(game PUBLIC MACOS)
endif (APPLE)

if (MSVC)
	set_property(TARGET game APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
