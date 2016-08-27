cmake_minimum_required(VERSION 2.6)

set(HAS_LIBIPHONE 1 PARENT_SCOPE)
message("setting HAS_LIBIPHONE")

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../libgg libgg)
endif (NOT HAS_LIBGG)

#

project(libiphone)

# --- libiphone ---

set(source Event_SDL.cpp EventManager.cpp TouchDLG.cpp TouchInfo.cpp TouchMgr.cpp)

add_library(libiphone ${source})

target_compile_definitions(libiphone PRIVATE _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
target_compile_definitions(libiphone PUBLIC $<$<CONFIG:Debug>:DEBUG>)
target_include_directories(libiphone PUBLIC "${PROJECT_SOURCE_DIR}")
target_include_directories(libiphone PRIVATE "../users/marcel/framework/win32/include")
target_include_directories(libiphone PUBLIC "${PROJECT_SOURCE_DIR}/../libgg") # because CMake sucks and only imports the PUBLIC include directories if you're DIRECTLY linking against a library

add_dependencies(libiphone libgg)

if (MSVC)
	set_property(TARGET libiphone APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
