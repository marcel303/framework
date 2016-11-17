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

project(anaglyphic)

# --- anaglyphic ---

file(GLOB_RECURSE source "*.cpp" "*.h")

set(GAME_FILES
	main.cpp)
source_group("Game Files" FILES ${GAME_FILES})

add_executable(anaglyphic ${source})

target_compile_definitions(anaglyphic PRIVATE NO_SDL_GLEXT HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX)

target_include_directories(anaglyphic PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(anaglyphic PRIVATE framework)
target_link_libraries(anaglyphic PRIVATE libgg)

target_link_libraries(anaglyphic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/FreeImage.lib)
target_link_libraries(anaglyphic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/freetype252.lib)
target_link_libraries(anaglyphic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/glew32.lib)
target_link_libraries(anaglyphic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/OpenAL32.lib)
target_link_libraries(anaglyphic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/SDL2.lib)
target_link_libraries(anaglyphic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/SDL2main.lib)
target_link_libraries(anaglyphic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/XInput9_1_0.lib)
target_link_libraries(anaglyphic PRIVATE Winmm.lib OpenGL32.lib WSock32.lib Ws2_32.lib)

if (MSVC)
	set_property(TARGET anaglyphic APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
