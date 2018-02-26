cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 11)

set(HAS_LIBGG 0)
set(HAS_LIBOSC 0)
set(HAS_LIBPS3EYE 0)
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
if (NOT HAS_LIBPS3EYE)
	message("including libps3eye")
	add_subdirectory(../../../libps3eye libps3eye)
endif (NOT HAS_LIBPS3EYE)
if (NOT HAS_FRAMEWORK)
	message("including framework")
	add_subdirectory(../../../framework framework)
endif (NOT HAS_FRAMEWORK)

#

project(veem-camapp)

# --- veem-camapp ---

file(GLOB_RECURSE source "*.cpp" "*.h")
file(GLOB resources "${CMAKE_CURRENT_SOURCE_DIR}/calibri.ttf")

source_group("sources" FILES ${source})

set_source_files_properties(${resources} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

add_executable(veem-camapp MACOSX_BUNDLE ${source} ${resources})

target_include_directories(veem-camapp PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(veem-camapp PRIVATE framework)
target_link_libraries(veem-camapp PRIVATE libps3eye)
target_link_libraries(veem-camapp PRIVATE libosc)
target_link_libraries(veem-camapp PRIVATE libgg)

target_link_libraries(veem-camapp PRIVATE ${FRAMEWORK_LIBRARIES})
