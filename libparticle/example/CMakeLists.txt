cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 14)

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../../libgg libgg)
endif (NOT HAS_LIBGG)
if (NOT HAS_LIBPARTICLE)
	message("including libparticle")
	add_subdirectory(../libparticle)
endif (NOT HAS_LIBPARTICLE)
#if (NOT HAS_FRAMEWORK)
#	message("including framework")
#	add_subdirectory(../../framework framework)
#endif (NOT HAS_FRAMEWORK)

#

project(libparticle_test)

# --- libparticle_test ---

file(GLOB_RECURSE source "*.cpp" "*.h")

add_executable(libparticle_test ${source})

target_compile_definitions(libparticle_test PRIVATE NO_SDL_GLEXT HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)

target_link_libraries(libparticle_test PRIVATE libparticle)
target_link_libraries(libparticle_test PRIVATE framework)
target_link_libraries(libparticle_test PRIVATE libnfd)
target_link_libraries(libparticle_test PRIVATE libgg)

target_link_libraries(libparticle_test PRIVATE ${FRAMEWORK_LIBRARIES})

if (APPLE)
	target_compile_definitions(libparticle_test PUBLIC MACOS)
endif (APPLE)

if (MSVC)
	set_property(TARGET libparticle_test APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
