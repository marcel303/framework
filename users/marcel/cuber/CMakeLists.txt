cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 14)

set(HAS_LIBGG 0)
set(HAS_FRAMEWORK 0)

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../../../libgg libgg)
endif (NOT HAS_LIBGG)
if (NOT HAS_FRAMEWORK)
	message("including framework")
	add_subdirectory(../../../framework framework)
endif (NOT HAS_FRAMEWORK)

#

project(cuber)

# --- cuber ---

file(GLOB source RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cpp" "*.h")
list(REMOVE_ITEM source script.cpp)

add_executable(cuber ${source})

target_compile_definitions(cuber PRIVATE NO_SDL_GLEXT HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX)

target_include_directories(cuber PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(cuber PRIVATE framework)
target_link_libraries(cuber PRIVATE libgg)

if (APPLE)
	target_compile_definitions(cuber PUBLIC MACOS)

	find_library(COCOA_LIBRARY Cocoa)
	find_library(CARBON_LIBRARY Carbon)
	find_library(COREAUDIO_LIBRARY CoreAudio)
	find_library(COREVIDEO_LIBRARY CoreVideo)
	find_library(IOKIT_LIBRARY IOKit)
	find_library(AUDIOTOOLBOX_LIBRARY AudioToolbox)
	find_library(AUDIOUNIT_LIBRARY AudioUnit)
	find_library(OPENAL_LIBRARY OpenAL)
	find_library(OPENGL_LIBRARY OpenGL)
	find_library(FF_LIBRARY ForceFeedback)
	find_library(ICONV_LIBRARY iconv)
	find_library(FREETYPE_LIBRARY freetype2)
	find_library(BZ2_LIBRARY bz2)
	
	target_link_libraries(cuber PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../framework/macos/glew/lib/libGLEW.a)
	target_link_libraries(cuber PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../framework/macos/lib/libfreeimage.a)
	target_link_libraries(cuber PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../framework/macos/lib/libfreetype.a)
	target_link_libraries(cuber PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../framework/macos/lib/libSDL2.a)
	target_link_libraries(cuber PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../framework/macos/lib/libSDL2main.a)
	target_link_libraries(cuber PRIVATE ${COCOA_LIBRARY})
	target_link_libraries(cuber PRIVATE ${CARBON_LIBRARY})
	target_link_libraries(cuber PRIVATE ${COREAUDIO_LIBRARY})
	target_link_libraries(cuber PRIVATE ${COREVIDEO_LIBRARY})
	target_link_libraries(cuber PRIVATE ${IOKIT_LIBRARY})
	target_link_libraries(cuber PRIVATE ${AUDIOTOOLBOX_LIBRARY})
	target_link_libraries(cuber PRIVATE ${AUDIOUNIT_LIBRARY})
	target_link_libraries(cuber PRIVATE ${OPENAL_LIBRARY})
	target_link_libraries(cuber PRIVATE ${OPENGL_LIBRARY})
	target_link_libraries(cuber PRIVATE ${FF_LIBRARY})
	target_link_libraries(cuber PRIVATE ${ICONV_LIBRARY})
	target_link_libraries(cuber PRIVATE ${BZ2_LIBRARY})
endif (APPLE)

if (MSVC)
	target_link_libraries(cuber PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../framework/win32/lib/FreeImage.lib)
	target_link_libraries(cuber PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../framework/win32/lib/freetype252.lib)
	target_link_libraries(cuber PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../framework/win32/lib/glew32.lib)
	target_link_libraries(cuber PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../framework/win32/lib/OpenAL32.lib)
	target_link_libraries(cuber PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../framework/win32/lib/SDL2.lib)
	target_link_libraries(cuber PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../framework/win32/lib/SDL2main.lib)
	target_link_libraries(cuber PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../framework/win32/lib/XInput9_1_0.lib)
	target_link_libraries(cuber PRIVATE Winmm.lib OpenGL32.lib WSock32.lib)
endif (MSVC)

if (MSVC)
	set_property(TARGET cuber APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)

# --- script ---

set(WINDOWS_EXPORT_ALL_SYMBOLS, 1)

add_library(script SHARED OpenSimplexNoise.cpp OpenSimplexNoise.h script.cpp script.h simplexnoise.cpp simplexnoise.h)

target_compile_definitions(script PRIVATE NO_SDL_GLEXT HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX)

if (APPLE)
	target_include_directories(script PRIVATE "${PROJECT_SOURCE_DIR}")
	target_include_directories(script PRIVATE "${PROJECT_SOURCE_DIR}/../../../framework/macos/include")
endif (APPLE)

if (MSVC)
	target_include_directories(script PRIVATE "${PROJECT_SOURCE_DIR}")
	target_include_directories(script PRIVATE "${PROJECT_SOURCE_DIR}/../../../framework/win32/include")
endif (MSVC)

target_link_libraries(script PRIVATE libgg)

if (MSVC)
	set_property(TARGET script APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
