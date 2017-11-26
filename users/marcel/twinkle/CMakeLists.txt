cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 11)

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
	add_subdirectory(../../../framework framework)
endif (NOT HAS_FRAMEWORK)

#

project(twinkle)

# --- twinkle ---

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
	hud.cpp
	hud.h
	levelevents.cpp
	main.cpp
	player.cpp
	player.h)
source_group("Game Files" FILES ${GAME_FILES})

add_executable(twinkle ${source})

target_compile_definitions(twinkle PRIVATE NO_SDL_GLEXT HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX)

target_include_directories(twinkle PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(twinkle PRIVATE framework)
target_link_libraries(twinkle PRIVATE libnet)
target_link_libraries(twinkle PRIVATE libgg)

target_link_libraries(twinkle PRIVATE ${FRAMEWORK_LIBRARIES})

if (MSVC)
	set_property(TARGET twinkle APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
