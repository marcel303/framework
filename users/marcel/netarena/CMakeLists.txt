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

project(riposte)

# --- riposte ---

file(GLOB source "*.cpp" "*.h")

set(GAME_FILES
	bullet.cpp
	bullet.h
	gamedefs.cpp
	gamedefs.h
	gamesim.cpp
	gamesim.h
	gametypes.cpp
	gametypes.h
	levelevents.cpp
	main.cpp
	player.cpp
	player.h)
source_group("Game Files" FILES ${GAME_FILES})

add_executable(riposte ${source})

target_compile_definitions(riposte PRIVATE NO_SDL_GLEXT HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)

target_include_directories(riposte PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(riposte PRIVATE framework)
target_link_libraries(riposte PRIVATE libnet)
target_link_libraries(riposte PRIVATE libgg)

target_link_libraries(riposte PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/FreeImage.lib)
target_link_libraries(riposte PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/freetype252.lib)
target_link_libraries(riposte PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/glew32.lib)
target_link_libraries(riposte PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/OpenAL32.lib)
target_link_libraries(riposte PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/SDL2.lib)
target_link_libraries(riposte PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/SDL2main.lib)
target_link_libraries(riposte PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/XInput9_1_0.lib)
target_link_libraries(riposte PRIVATE Winmm.lib OpenGL32.lib WSock32.lib)

if (MSVC)
	set_property(TARGET riposte APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
