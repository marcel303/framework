cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 11)

set(HAS_LIBGG 0)
set(HAS_FRAMEWORK 0)
set(HAS_JSUSFX_FRAMEWORK 0)

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../../../libgg libgg)
endif (NOT HAS_LIBGG)
if (NOT HAS_FRAMEWORK)
	message("including framework")
	add_subdirectory(../../../framework framework)
endif (NOT HAS_FRAMEWORK)
if (NOT HAS_JSUSFX_FRAMEWORK)
	message("including jsusfx-framework")
	add_subdirectory(../../../libjsusfx-framework jsusfx-framework)
endif (NOT HAS_JSUSFX_FRAMEWORK)

#

project(jsfx-framework-test)

# --- jsusfx-framework-test ---

file(GLOB source "*.cpp" "*.h")

source_group("sources" FILES ${source})

add_executable(jsusfx-framework-test ${source})

target_link_libraries(jsusfx-framework-test PRIVATE jsusfx-framework)
target_link_libraries(jsusfx-framework-test PRIVATE framework)
target_link_libraries(jsusfx-framework-test PRIVATE libgg)

target_link_libraries(jsusfx-framework-test PRIVATE ${FRAMEWORK_LIBRARIES})
