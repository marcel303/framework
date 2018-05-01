cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.11")

set(HAS_LIBGG 0)
set(HAS_LIBOSC 0)
set(HAS_FRAMEWORK 0)

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../../../libgg libgg)
endif (NOT HAS_LIBGG)
if (NOT HAS_LIBOSC)
	message("including libosc")
	add_subdirectory(../../../libosc libosc)
endif (NOT HAS_LIBOSC)
if (NOT HAS_FRAMEWORK)
	message("including framework")
	add_subdirectory(../../../framework framework)
endif (NOT HAS_FRAMEWORK)

#

project(imu9250)

# --- imu9250 ---

file(GLOB source "*.c" "*.cpp" "*.h" "*.hpp" "*.mm")
file(GLOB resources "data/*.ttf" "data/*.jpg")

source_group("sources" FILES ${source})

set_source_files_properties(${resources} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

add_executable(imu9250 MACOSX_BUNDLE ${source} ${resources})

if (APPLE)
	# copy dylib and framework files to the Debug and Release folders

	file(COPY ${FRAMEWORK_BINARIES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)

	file(COPY ${FRAMEWORK_BINARIES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
endif (APPLE)

target_include_directories(imu9250 PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(imu9250 PRIVATE framework)
target_link_libraries(imu9250 PRIVATE libosc)
target_link_libraries(imu9250 PRIVATE libgg)

target_link_libraries(imu9250 PRIVATE ${FRAMEWORK_LIBRARIES})

if (APPLE)
	find_library(IOBLUETOOH_LIBRARY IOBluetooth)
	target_link_libraries(imu9250 PRIVATE ${IOBLUETOOH_LIBRARY})
endif (APPLE)
