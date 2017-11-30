cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 14)

set(HAS_LIBPARTICLE 1 PARENT_SCOPE)
message("setting HAS_LIBPARTICLE")

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../libgg libgg)
endif (NOT HAS_LIBGG)
if (NOT HAS_LIBNFD)
	message("including libnfd")
	add_subdirectory(../libnfd libnfd)
endif (NOT HAS_LIBNFD)
if (NOT HAS_FRAMEWORK)
	message("including framework")
	add_subdirectory(../framework framework)
endif (NOT HAS_FRAMEWORK)

#

project(libparticle)

# --- libparticle ---

file(GLOB source "*.cpp" "*.h")

source_group("sources" FILES ${source})

add_library(libparticle ${source})

target_include_directories(libparticle PUBLIC "${PROJECT_SOURCE_DIR}")

target_link_libraries(libparticle framework)
target_link_libraries(libparticle libnfd)
target_link_libraries(libparticle libgg)
