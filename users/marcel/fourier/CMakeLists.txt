cmake_minimum_required(VERSION 2.6)

set(HAS_LIBGG 0)
set(HAS_FRAMEWORK 0)

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../../../libgg libgg)
endif (NOT HAS_LIBGG)
if (NOT HAS_FRAMEWORK)
	message("including framework")
	add_subdirectory(../framework framework)
endif (NOT HAS_FRAMEWORK)

#

project(fourier)

# --- fourier ---

file(GLOB source "*.cpp" "*.h")

source_group("sources" FILES ${source})

add_executable(fourier ${source})

target_include_directories(fourier PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(fourier PRIVATE framework)
target_link_libraries(fourier PRIVATE libgg)

target_link_libraries(fourier PRIVATE ${FRAMEWORK_LIBRARIES})

if (MSVC)
	set_property(TARGET fourier APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
