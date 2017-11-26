cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 14)

set(HAS_LIBGG 0)
set(HAS_FRAMEWORK 0)
set(HAS_LIBVIDEO 0)

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../../../libgg libgg)
endif (NOT HAS_LIBGG)
if (NOT HAS_FRAMEWORK)
	message("including framework")
	add_subdirectory(../../../framework framework)
endif (NOT HAS_FRAMEWORK)
if (NOT HAS_LIBVIDEO)
	message("including libvideo")
	add_subdirectory(../../../libvideo libvideo)
endif (NOT HAS_LIBVIDEO)

#

project(colorpart)

# --- colorpart ---

file(GLOB_RECURSE source "*.cpp" "*.h")

add_executable(colorpart ${source})

target_compile_definitions(colorpart PRIVATE NO_SDL_GLEXT HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX)

target_include_directories(colorpart PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(colorpart PRIVATE framework)
target_link_libraries(colorpart PRIVATE libgg)
target_link_libraries(colorpart PRIVATE libvideo)

target_link_libraries(colorpart PRIVATE ${FRAMEWORK_LIBRARIES})
target_link_libraries(colorpart PRIVATE ${LIBVIDEO_LIBRARIES})

if (MSVC)
	set_property(TARGET colorpart APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
