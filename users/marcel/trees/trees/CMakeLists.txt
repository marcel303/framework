cmake_minimum_required(VERSION 2.6)

project(trees)

# --- trees ---

file(GLOB source RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cpp" "*.h")

add_executable(trees ${source})

target_compile_definitions(trees PRIVATE _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
target_include_directories(trees PUBLIC "${PROJECT_SOURCE_DIR}")

if (MSVC)
	set_property(TARGET trees APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
