cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 14)

set(HAS_LIBPS3EYE 1 PARENT_SCOPE)
message("setting HAS_LIBPS3EYE")

# --- dependencies ---

#

project(libps3eye)

# --- libps3eye ---

file(GLOB_RECURSE source "*.*")

add_library(libps3eye ${source})

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules)

find_package(PkgConfig)
find_package(LibUSB REQUIRED)

target_include_directories(libps3eye PUBLIC "${PROJECT_SOURCE_DIR}")
target_include_directories(libps3eye PRIVATE "${PROJECT_SOURCE_DIR}" "${LibUSB_INCLUDE_DIRS}")

target_link_libraries(libps3eye PRIVATE ${LibUSB_LIBRARIES})

