cmake_minimum_required(VERSION 2.6)

set(HAS_LIBGG 0)
set(HAS_LIBAGG 0)
set(HAS_LIBCXIMAGE 0)
set(HAS_LIBKLODDER 0)
set(HAS_LIBTINYXML 0)

# --- dependencies ---

if (NOT HAS_LIBKLODDER)
	message("including libklodder")
	add_subdirectory(../libklodder libklodder)
endif (NOT HAS_LIBKLODDER)

if (NOT HAS_LIBTINYXML)
	message("including libtinyxml")
	add_subdirectory(../libtinyxml libtinyxml)
endif (NOT HAS_LIBTINYXML)

#

project(klodder)

# --- klodder ---

file(GLOB source RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cpp" "*.h")
#list(REMOVE_ITEM source script.cpp)

add_executable(klodder ${source})

target_compile_definitions(klodder PRIVATE NO_SDL_GLEXT HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX SETTINGS_FILESYSTEM USE_AGG)

target_include_directories(klodder PRIVATE "${PROJECT_SOURCE_DIR}")

#target_link_libraries(klodder PRIVATE libgg)
#target_link_libraries(klodder PRIVATE libagg)
#target_link_libraries(klodder PRIVATE libcximage)
target_link_libraries(klodder PRIVATE libklodder)
target_link_libraries(klodder PRIVATE libtinyxml)

#target_link_libraries(klodder PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/FreeImage.lib)
#target_link_libraries(klodder PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/freetype252.lib)
#target_link_libraries(klodder PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/glew32.lib)
#target_link_libraries(klodder PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/OpenAL32.lib)
#target_link_libraries(klodder PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/SDL2.lib)
#target_link_libraries(klodder PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/SDL2main.lib)
#target_link_libraries(klodder PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/XInput9_1_0.lib)
#target_link_libraries(klodder PRIVATE Winmm.lib OpenGL32.lib WSock32.lib)

if (MSVC)
	set_property(TARGET klodder APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
