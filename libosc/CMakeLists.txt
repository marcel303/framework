cmake_minimum_required(VERSION 2.6)

set(HAS_LIBOSC 1 PARENT_SCOPE)
message("setting HAS_LIBOSC")

# --- dependencies ---

#

project(libosc)

# --- libosc ---

file(GLOB_RECURSE source "*.c" "*.cpp" "*.h")

if (MSVC)
	file(GLOB_RECURSE source_posix "ip/posix/*.*")
	list(REMOVE_ITEM source ${source_posix})
else (MSVC)
	file(GLOB_RECURSE source_win32 "ip/win32/*.*")
	list(REMOVE_ITEM source ${source_win32})
endif (MSVC)

source_group("sources" FILES ${source})

add_library(libosc ${source})

target_compile_definitions(libosc PRIVATE _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
target_include_directories(libosc PUBLIC "${PROJECT_SOURCE_DIR}")

if (MSVC)
	set_property(TARGET libosc APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
