cmake_minimum_required(VERSION 2.6)

set(HAS_LIBPARTICLE 1 PARENT_SCOPE)
message("setting HAS_LIBPARTICLE")

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../../../libgg libgg)
endif (NOT HAS_LIBGG)
if (NOT HAS_LIBNFD)
	message("including libnfd")
	add_subdirectory(../../../libnfd libnfd)
endif (NOT HAS_LIBNFD)
if (NOT HAS_FRAMEWORK)
	message("including framework")
	add_subdirectory(../framework framework)
endif (NOT HAS_FRAMEWORK)

#

project(libparticle)

# --- libparticle ---

file(GLOB source "*.cpp" "*.h")

add_library(libparticle ${source})

target_compile_definitions(libparticle PRIVATE NO_SDL_GLEXT HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)

target_include_directories(libparticle PUBLIC "${PROJECT_SOURCE_DIR}")
target_include_directories(libparticle PUBLIC "${PROJECT_SOURCE_DIR}/../framework") # because CMake sucks and only imports the PUBLIC include directories if you're DIRECTLY linking against a library
target_include_directories(libparticle PUBLIC "${PROJECT_SOURCE_DIR}/../framework/win32/include") # because CMake sucks and only imports the PUBLIC include directories if you're DIRECTLY linking against a library
target_include_directories(libparticle PUBLIC "${PROJECT_SOURCE_DIR}/../../../libgg") # because CMake sucks and only imports the PUBLIC include directories if you're DIRECTLY linking against a library
target_include_directories(libparticle PUBLIC "${PROJECT_SOURCE_DIR}/../../../libnfd") # because CMake sucks and only imports the PUBLIC include directories if you're DIRECTLY linking against a library

add_dependencies(libparticle framework)
add_dependencies(libparticle libnfd)
add_dependencies(libparticle libgg)

if (MSVC)
	set_property(TARGET libparticle APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)

# --- libparticle_test ---

add_executable(libparticle_test ${source})

target_compile_definitions(libparticle_test PRIVATE NO_SDL_GLEXT HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)

target_link_libraries(libparticle_test PRIVATE framework)
target_link_libraries(libparticle_test PRIVATE libnfd)
target_link_libraries(libparticle_test PRIVATE libgg)

target_link_libraries(libparticle_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/FreeImage.lib)
target_link_libraries(libparticle_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/freetype252.lib)
target_link_libraries(libparticle_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/glew32.lib)
target_link_libraries(libparticle_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/OpenAL32.lib)
target_link_libraries(libparticle_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/SDL2.lib)
target_link_libraries(libparticle_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/SDL2main.lib)
target_link_libraries(libparticle_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/XInput9_1_0.lib)
target_link_libraries(libparticle_test PRIVATE Winmm.lib OpenGL32.lib WSock32.lib)

if (MSVC)
	set_property(TARGET libparticle_test APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
