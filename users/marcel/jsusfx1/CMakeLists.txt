cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 11)

set(HAS_LIBGG 0)
set(HAS_FRAMEWORK 0)

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../../../libgg libgg)
endif (NOT HAS_LIBGG)
if (NOT HAS_FRAMEWORK)
	message("including framework")
	add_subdirectory(../../../framework framework)
endif (NOT HAS_FRAMEWORK)

add_subdirectory(../../../../jsusfx jsusfx)

#

project(jsfx-framework)

# --- jsusfx-framework ---

file(GLOB source "*.cpp" "*.h")

source_group("sources" FILES ${source})

add_executable(jsusfx-framework ${source})

target_link_libraries(jsusfx-framework PRIVATE jsusfx)
target_link_libraries(jsusfx-framework PRIVATE framework)
target_link_libraries(jsusfx-framework PRIVATE libgg)

target_link_libraries(jsusfx-framework PRIVATE ${FRAMEWORK_LIBRARIES})
