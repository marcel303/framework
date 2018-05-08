cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 11)

set(HAS_LIBGG 0)
set(HAS_LIBOSC 0)
set(HAS_LIBPS3EYE 0)
set(HAS_FRAMEWORK 0)
set(HAS_LIBPARTICLE 0)

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
if (NOT HAS_LIBPARTICLE)
	message("including libparticle")
	add_subdirectory(../../../libparticle libparticle)
endif (NOT HAS_LIBPARTICLE)

#

project(ps3eye-to-osc)

# --- ps3eye-to-osc ---

file(GLOB source "*.cpp" "*.h")
file(GLOB resources "${CMAKE_CURRENT_SOURCE_DIR}/calibri.ttf")

source_group("sources" FILES ${source})

set_source_files_properties(${resources} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

add_executable(ps3eye-to-osc MACOSX_BUNDLE ${source} ${resources})

target_include_directories(ps3eye-to-osc PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(ps3eye-to-osc PRIVATE libparticle)
target_link_libraries(ps3eye-to-osc PRIVATE framework)
target_link_libraries(ps3eye-to-osc PRIVATE libps3eye)
target_link_libraries(ps3eye-to-osc PRIVATE libosc)
target_link_libraries(ps3eye-to-osc PRIVATE libgg)

target_link_libraries(ps3eye-to-osc PRIVATE ${FRAMEWORK_LIBRARIES})
