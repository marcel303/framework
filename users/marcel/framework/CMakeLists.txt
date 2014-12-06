cmake_minimum_required(VERSION 2.6)

set(HAS_FRAMEWORK 1 PARENT_SCOPE)
message("setting HAS_FRAMEWORK")

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../../../libgg libgg)
endif (NOT HAS_LIBGG)

#

project(framework)

# --- framework ---

file(GLOB source RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/audiostream/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/audiostream/*.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/audiostream/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/../fbx1/*.cpp")
list(REMOVE_ITEM source aniplay.cpp)
list(REMOVE_ITEM source example.cpp)
list(REMOVE_ITEM source ../fbx1/main.cpp)

add_library(framework ${source})

target_compile_definitions(framework PRIVATE NO_SDL_GLEXT HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)

target_include_directories(framework PUBLIC "${PROJECT_SOURCE_DIR}")
target_include_directories(framework PRIVATE "${PROJECT_SOURCE_DIR}/audiostream")
target_include_directories(framework PUBLIC "${PROJECT_SOURCE_DIR}/win32/include")
target_include_directories(framework PRIVATE "${PROJECT_SOURCE_DIR}/win32/include/freetype")
target_include_directories(framework PUBLIC "${PROJECT_SOURCE_DIR}/../../../libgg") # because CMake sucks and only imports the PUBLIC include directories if you're DIRECTLY linking against a library

add_dependencies(framework libgg)

if (MSVC)
	set_property(TARGET framework APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)


# --- test application ---

file(GLOB source RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/example.cpp")

add_executable(framework_test ${source})

target_include_directories(framework_test PRIVATE "${PROJECT_SOURCE_DIR}/win32/include")

target_link_libraries(framework_test PRIVATE framework)
target_link_libraries(framework_test PRIVATE libgg)
target_link_libraries(framework_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/FreeImage.lib)
target_link_libraries(framework_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/freetype252.lib)
target_link_libraries(framework_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/glew32.lib)
target_link_libraries(framework_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/OpenAL32.lib)
target_link_libraries(framework_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/SDL2.lib)
target_link_libraries(framework_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/SDL2main.lib)
target_link_libraries(framework_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/XInput9_1_0.lib)
target_link_libraries(framework_test PRIVATE OpenGL32.lib)

# --- animation player ---

file(GLOB source RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/aniplay.cpp")

add_executable(aniplay ${source})

target_include_directories(framework_test PRIVATE "${PROJECT_SOURCE_DIR}/win32/include")

target_link_libraries(aniplay PRIVATE framework)
target_link_libraries(aniplay PRIVATE libgg)
target_link_libraries(aniplay PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/FreeImage.lib)
target_link_libraries(aniplay PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/freetype252.lib)
target_link_libraries(aniplay PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/glew32.lib)
target_link_libraries(aniplay PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/OpenAL32.lib)
target_link_libraries(aniplay PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/SDL2.lib)
target_link_libraries(aniplay PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/SDL2main.lib)
target_link_libraries(aniplay PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/XInput9_1_0.lib)
target_link_libraries(aniplay PRIVATE OpenGL32.lib)
