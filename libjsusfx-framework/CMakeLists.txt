cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 14)

set(HAS_JSUSFX_FRAMEWORK 1 PARENT_SCOPE)
message("setting HAS_JSUSFX_FRAMEWORK")

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../libgg libgg)
endif (NOT HAS_LIBGG)
if (NOT HAS_FRAMEWORK)
	message("including framework")
	add_subdirectory(../framework framework)
endif (NOT HAS_FRAMEWORK)

add_subdirectory(../libjsusfx/src jsusfx)

#

project(jsusfx-framework)

# -- framework + JSUSFX library --

set(sources
	gfx-framework.cpp
	jsusfx-framework.cpp)

file(GLOB_RECURSE headers *.h)

source_group("sources" FILES ${sources})
source_group("headers" FILES ${headers})

add_library(jsusfx-framework ${sources} ${headers})

target_include_directories(jsusfx-framework PUBLIC "${PROJECT_SOURCE_DIR}")

target_link_libraries(jsusfx-framework PRIVATE jsusfx)
target_link_libraries(jsusfx-framework PRIVATE framework)
target_link_libraries(jsusfx-framework PRIVATE libgg)
