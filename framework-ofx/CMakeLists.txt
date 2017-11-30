cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 14)

set(HAS_FRAMEWORK_OFX 1 PARENT_SCOPE)
message("setting HAS_FRAMEWORK_OFX")

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

project(framework-ofx)

# --- framework-ofx ---

file(GLOB_RECURSE source "*.*")

source_group("sources" FILES ${source})

add_library(framework-ofx ${source})

target_include_directories(framework-ofx PUBLIC "${PROJECT_SOURCE_DIR}")

target_link_libraries(framework-ofx libgg)
target_link_libraries(framework-ofx framework)
