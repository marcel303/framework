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

if (MSVC)
	target_link_libraries(libparticle_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/FreeImage.lib)
	target_link_libraries(libparticle_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/freetype252.lib)
	target_link_libraries(libparticle_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/glew32.lib)
	target_link_libraries(libparticle_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/OpenAL32.lib)
	target_link_libraries(libparticle_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/SDL2.lib)
	target_link_libraries(libparticle_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/SDL2main.lib)
	target_link_libraries(libparticle_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/XInput9_1_0.lib)
	target_link_libraries(libparticle_test PRIVATE Winmm.lib OpenGL32.lib WSock32.lib)
endif (MSVC)

if (APPLE)
	target_compile_definitions(libparticle_test PUBLIC MACOS)

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
	
	target_link_libraries(libparticle_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../framework/macos/glew/lib/libGLEW.a)
	target_link_libraries(libparticle_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../framework/macos/lib/libfreeimage.a)
	target_link_libraries(libparticle_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../framework/macos/lib/libfreetype.a)
	target_link_libraries(libparticle_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../framework/macos/lib/libSDL2.a)
	target_link_libraries(libparticle_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../framework/macos/lib/libSDL2main.a)
	target_link_libraries(libparticle_test PRIVATE ${COCOA_LIBRARY})
	target_link_libraries(libparticle_test PRIVATE ${CARBON_LIBRARY})
	target_link_libraries(libparticle_test PRIVATE ${COREAUDIO_LIBRARY})
	target_link_libraries(libparticle_test PRIVATE ${COREVIDEO_LIBRARY})
	target_link_libraries(libparticle_test PRIVATE ${IOKIT_LIBRARY})
	target_link_libraries(libparticle_test PRIVATE ${AUDIOTOOLBOX_LIBRARY})
	target_link_libraries(libparticle_test PRIVATE ${AUDIOUNIT_LIBRARY})
	target_link_libraries(libparticle_test PRIVATE ${OPENAL_LIBRARY})
	target_link_libraries(libparticle_test PRIVATE ${OPENGL_LIBRARY})
	target_link_libraries(libparticle_test PRIVATE ${FF_LIBRARY})
	target_link_libraries(libparticle_test PRIVATE ${ICONV_LIBRARY})
	target_link_libraries(libparticle_test PRIVATE ${BZ2_LIBRARY})
endif (APPLE)

if (MSVC)
	set_property(TARGET libparticle_test APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
