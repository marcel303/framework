cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 14)

set(HAS_LIBGG 0)
set(HAS_LIBOSC 0)
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

project(4dtest)

# --- 4dtest ---

file(GLOB_RECURSE source "*.cpp" "*.h")

file(GLOB_RECURSE source_ui "../libparticle/ui.cpp" "../libparticle/ui.h" "../libparticle/textfield.cpp" "../libparticle/textfield.h" "../libparticle/colorwheel.cpp" "../libparticle/colorwheel.h" "../libparticle/particle.cpp" "../libparticle/particle.h")

list(APPEND source ${source_ui})

source_group("4DTest" FILES ${source})
source_group("ui" FILES ${source_ui})

add_executable(4dtest ${source})

target_compile_definitions(4dtest PRIVATE NO_SDL_GLEXT)

if (WIN32)
	target_compile_definitions(4dtest PRIVATE HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX)
	
	# copy DLL files to the Debug and Release folders
	
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/dll/FreeImage.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/dll/glew32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/dll/OpenAL32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/dll/SDL2.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/dll/XInput9_1_0.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../avpaint/win32/dll/portaudio_x86.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/dll/FreeImage.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/dll/glew32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/dll/OpenAL32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/dll/SDL2.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/dll/XInput9_1_0.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../avpaint/win32/dll/portaudio_x86.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
endif (WIN32)

target_include_directories(4dtest PRIVATE "${PROJECT_SOURCE_DIR}")

if (APPLE)
	target_include_directories(4dtest PRIVATE "../avgraph/macos/include")
	target_include_directories(4dtest PRIVATE "../vfxpro/macos/include")
endif (APPLE)

if (WIN32)
	target_include_directories(4dtest PRIVATE "../vfxpro/win32/include")
	target_include_directories(4dtest PRIVATE "../avpaint/win32/include")
endif (WIN32)

target_link_libraries(4dtest PRIVATE framework)
target_link_libraries(4dtest PRIVATE libgg)

if (MSVC)
	target_link_libraries(4dtest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../avpaint/win32/lib/portaudio_x86.lib)
	
	target_link_libraries(4dtest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/FreeImage.lib)
	target_link_libraries(4dtest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/freetype252.lib)
	target_link_libraries(4dtest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/glew32.lib)
	target_link_libraries(4dtest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/OpenAL32.lib)
	target_link_libraries(4dtest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/SDL2.lib)
	target_link_libraries(4dtest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/SDL2main.lib)
	target_link_libraries(4dtest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/XInput9_1_0.lib)
	target_link_libraries(4dtest PRIVATE Winmm.lib OpenGL32.lib WSock32.lib Ws2_32.lib)
endif (MSVC)

if (APPLE)
	target_compile_definitions(4dtest PUBLIC MACOS)

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
	find_library(BZ2_LIBRARY bz2)
	
	target_link_libraries(4dtest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/macos/glew/lib/libGLEW.a)
	target_link_libraries(4dtest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/macos/lib/libfreeimage.a)
	target_link_libraries(4dtest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/macos/lib/libfreetype.a)
	target_link_libraries(4dtest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/macos/lib/libSDL2.a)
	target_link_libraries(4dtest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/macos/lib/libSDL2main.a)
	target_link_libraries(4dtest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../avpaint/macos/lib/libportaudio.a)
	target_link_libraries(4dtest PRIVATE ${COCOA_LIBRARY})
	target_link_libraries(4dtest PRIVATE ${CARBON_LIBRARY})
	target_link_libraries(4dtest PRIVATE ${COREAUDIO_LIBRARY})
	target_link_libraries(4dtest PRIVATE ${COREVIDEO_LIBRARY})
	target_link_libraries(4dtest PRIVATE ${IOKIT_LIBRARY})
	target_link_libraries(4dtest PRIVATE ${AUDIOTOOLBOX_LIBRARY})
	target_link_libraries(4dtest PRIVATE ${AUDIOUNIT_LIBRARY})
	target_link_libraries(4dtest PRIVATE ${OPENAL_LIBRARY})
	target_link_libraries(4dtest PRIVATE ${OPENGL_LIBRARY})
	target_link_libraries(4dtest PRIVATE ${FF_LIBRARY})
	target_link_libraries(4dtest PRIVATE ${ICONV_LIBRARY})
	target_link_libraries(4dtest PRIVATE ${BZ2_LIBRARY}) # FreeType
endif (APPLE)

if (MSVC)
	set_property(TARGET 4dtest APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
	set_property(TARGET 4dtest APPEND_STRING PROPERTY LINK_FLAGS /SAFESEH:NO)
endif (MSVC)
