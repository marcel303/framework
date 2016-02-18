cmake_minimum_required(VERSION 2.6)

set(HAS_LIBGG 0)
set(HAS_LIBNET 0)
set(HAS_FRAMEWORK 0)

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../../../libgg libgg)
endif (NOT HAS_LIBGG)
if (NOT HAS_LIBNET)
	message("including libnet")
	add_subdirectory(../../../libnet libnet)
endif (NOT HAS_LIBNET)
if (NOT HAS_FRAMEWORK)
	message("including framework")
	add_subdirectory(../framework framework)
endif (NOT HAS_FRAMEWORK)

#

project(vfxpro)

# --- twinkle ---

file(GLOB_RECURSE source "*.cpp" "*.h")
file(GLOB_RECURSE excluded_source "mediaplayer/*.*")
list(REMOVE_ITEM source ${excluded_source})

set(GAME_FILES
	main.cpp)
source_group("Game Files" FILES ${GAME_FILES})

add_executable(vfxpro ${source})

target_compile_definitions(vfxpro PRIVATE NO_SDL_GLEXT HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX)

target_include_directories(vfxpro PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(vfxpro PRIVATE framework)
target_link_libraries(vfxpro PRIVATE libnet)
target_link_libraries(vfxpro PRIVATE libgg)

target_link_libraries(vfxpro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/FreeImage.lib)
target_link_libraries(vfxpro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/freetype252.lib)
target_link_libraries(vfxpro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/glew32.lib)
target_link_libraries(vfxpro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/OpenAL32.lib)
target_link_libraries(vfxpro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/SDL2.lib)
target_link_libraries(vfxpro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/SDL2main.lib)
target_link_libraries(vfxpro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/XInput9_1_0.lib)
target_link_libraries(vfxpro PRIVATE Winmm.lib OpenGL32.lib WSock32.lib Ws2_32.lib)

if (MSVC)
	set_property(TARGET vfxpro APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
