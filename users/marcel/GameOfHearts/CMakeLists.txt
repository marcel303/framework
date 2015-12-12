cmake_minimum_required(VERSION 2.6)

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

project(GameOfHearts)

# --- GameOfHearts ---

file(GLOB_RECURSE source "*.cpp" "*.h")

add_executable(GameOfHearts ${source})

target_compile_definitions(GameOfHearts PRIVATE NO_SDL_GLEXT HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX)

target_include_directories(GameOfHearts PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(GameOfHearts PRIVATE framework)
target_link_libraries(GameOfHearts PRIVATE libgg)

target_link_libraries(GameOfHearts PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/FreeImage.lib)
target_link_libraries(GameOfHearts PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/freetype252.lib)
target_link_libraries(GameOfHearts PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/glew32.lib)
target_link_libraries(GameOfHearts PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/OpenAL32.lib)
target_link_libraries(GameOfHearts PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/SDL2.lib)
target_link_libraries(GameOfHearts PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/SDL2main.lib)
target_link_libraries(GameOfHearts PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/XInput9_1_0.lib)
target_link_libraries(GameOfHearts PRIVATE Winmm.lib OpenGL32.lib WSock32.lib)

if (MSVC)
	set_property(TARGET GameOfHearts APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
