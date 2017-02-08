cmake_minimum_required(VERSION 2.6)

set(HAS_LIBNET 1 PARENT_SCOPE)
message("setting HAS_LIBNET")

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../libgg libgg)
endif (NOT HAS_LIBGG)

#

project(libnet)

# --- libnet ---

file(GLOB source RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cpp" "*.h")
list(REMOVE_ITEM source test.cpp)

add_library(libnet ${source})

if (MSVC)
	target_compile_definitions(libnet PRIVATE _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
endif (MSVC)

target_include_directories(libnet PUBLIC "${PROJECT_SOURCE_DIR}")
target_include_directories(libnet PUBLIC "${PROJECT_SOURCE_DIR}/../libgg") # because CMake sucks and only imports the PUBLIC include directories if you're DIRECTLY linking against a library

add_dependencies(libnet libgg)

# --- test application ---

#file(GLOB source RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/test.cpp")
#
#add_executable(libnet_test ${source})
#
#target_include_directories(libnet_test PRIVATE "${PROJECT_SOURCE_DIR}/test")
#target_link_libraries(libnet_test PRIVATE libnet)
#target_link_libraries(libnet_test PRIVATE libgg)
#target_link_libraries(libnet_test PRIVATE SDL.lib SDLmain.lib WSock32.lib)
