cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 11)

set(HAS_LIBGG 0)
set(HAS_FRAMEWORK 0)

set(HAS_LIBNET 1 PARENT_SCOPE)
message("setting HAS_LIBNET")

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../libgg libgg)
endif (NOT HAS_LIBGG)
if (NOT HAS_FRAMEWORK)
	message("including framework")
	add_subdirectory(../framework framework)
endif (NOT HAS_FRAMEWORK)

#

project(libnet)

# --- libnet ---

file(GLOB source RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cpp" "*.h")

source_group("sources" FILES ${source})

add_library(libnet ${source})

target_include_directories(libnet PUBLIC "${PROJECT_SOURCE_DIR}")

target_link_libraries(libnet libgg)

# --- test application ---

file(GLOB source_test RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/test/test.cpp")

add_executable(libnet_test ${source_test})

target_include_directories(libnet_test PRIVATE "${PROJECT_SOURCE_DIR}/test")

target_link_libraries(libnet_test PRIVATE framework)
target_link_libraries(libnet_test PRIVATE libnet)
target_link_libraries(libnet_test PRIVATE libgg)

target_link_libraries(libnet_test PRIVATE ${FRAMEWORK_LIBRARIES})
