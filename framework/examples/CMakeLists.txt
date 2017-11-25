cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 14)

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../../libgg libgg)
endif (NOT HAS_LIBGG)
if (NOT HAS_FRAMEWORK)
	message("including framework")
	add_subdirectory(../../framework framework)
endif (NOT HAS_FRAMEWORK)

#

project(framework-tests)

# --- tests ---

add_executable(aniplay aniplay.cpp)
add_executable(example example.cpp)
add_executable(model model.cpp)
add_executable(sounds sounds.cpp)
add_executable(sprites sprites.cpp)

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
	target_compile_definitions(model PUBLIC MACOS)
	target_compile_definitions(sounds PUBLIC MACOS)
	target_compile_definitions(sprites PUBLIC MACOS)

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

	set(LIBRARIES
		framework
		libgg
		${CMAKE_CURRENT_SOURCE_DIR}/../../framework/macos/glew/lib/libGLEW.a
		${CMAKE_CURRENT_SOURCE_DIR}/../../framework/macos/lib/libfreeimage.a
		${CMAKE_CURRENT_SOURCE_DIR}/../../framework/macos/lib/libfreetype.a
		${CMAKE_CURRENT_SOURCE_DIR}/../../framework/macos/lib/libSDL2.a
		${CMAKE_CURRENT_SOURCE_DIR}/../../framework/macos/lib/libSDL2main.a
		${COCOA_LIBRARY}
		${CARBON_LIBRARY}
		${COREAUDIO_LIBRARY}
		${COREVIDEO_LIBRARY}
		${IOKIT_LIBRARY}
		${AUDIOTOOLBOX_LIBRARY}
		${AUDIOUNIT_LIBRARY}
		${OPENAL_LIBRARY}
		${OPENGL_LIBRARY}
		${FF_LIBRARY}
		${ICONV_LIBRARY}
		${BZ2_LIBRARY})

	target_link_libraries(aniplay PRIVATE ${LIBRARIES})
	target_link_libraries(example PRIVATE ${LIBRARIES})
	target_link_libraries(model PRIVATE ${LIBRARIES})
	target_link_libraries(sounds PRIVATE ${LIBRARIES})
	target_link_libraries(sprites PRIVATE ${LIBRARIES})
endif (APPLE)

if (MSVC)
	set_property(TARGET libparticle_test APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
