cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 11)

set(HAS_LIBJGMOD 1 PARENT_SCOPE)
message("setting HAS_LIBJGMOD")

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../../../libgg libgg)
endif (NOT HAS_LIBGG)
if (NOT HAS_FRAMEWORK)
	message("including framework")
	add_subdirectory(../../../framework framework)
endif (NOT HAS_FRAMEWORK)

#

project(libjgmod)

# --- libjgmod ---

file(GLOB source RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cpp" "*.h")
list(REMOVE_ITEM source jgm.cpp main.cpp)

source_group("sources" FILES ${source})

add_library(libjgmod ${source})

target_include_directories(libjgmod PUBLIC "${PROJECT_SOURCE_DIR}")

target_link_libraries(libjgmod PUBLIC framework)
target_link_libraries(libjgmod PUBLIC libgg)

#
#
#

add_executable(jgm jgm.cpp)

target_link_libraries(jgm PRIVATE ${FRAMEWORK_LIBRARIES})
target_link_libraries(jgm PRIVATE libjgmod)
target_link_libraries(jgm PRIVATE framework)

#
#
#

add_executable(jgmod main.cpp)

target_link_libraries(jgmod PRIVATE ${FRAMEWORK_LIBRARIES})
target_link_libraries(jgmod PRIVATE libjgmod)
target_link_libraries(jgmod PRIVATE framework)
