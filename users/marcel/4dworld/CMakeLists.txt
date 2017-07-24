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
if (NOT HAS_LIBOSC)
	message("including libosc")
	add_subdirectory(../../../libosc libosc)
endif (NOT HAS_LIBOSC)
if (NOT HAS_FRAMEWORK)
	message("including framework")
	add_subdirectory(../framework framework)
endif (NOT HAS_FRAMEWORK)

#

project(avgraph)

# --- avgraph ---

file(GLOB_RECURSE source "*.cpp" "*.h")
file(GLOB_RECURSE source_graph "../avgraph/graph.cpp" "../avgraph/graph.h")

file(GLOB_RECURSE source_audioNodes "audioNodes/*.*")
file(GLOB_RECURSE source_objects "objects/*.*" ${source_graph})
file(GLOB_RECURSE source_macos "macos/*.*")
file(GLOB_RECURSE source_leap "leap/*.*")
file(GLOB_RECURSE source_xmm "xmm/*.*")
file(GLOB_RECURSE source_delaunay "delaunay/*.*")

file(GLOB source_tests "test*.*")

file(GLOB_RECURSE source_ui "../libparticle/ui.cpp" "../libparticle/ui.h" "../libparticle/textfield.cpp" "../libparticle/textfield.h" "../libparticle/colorwheel.cpp" "../libparticle/colorwheel.h" "../libparticle/particle.cpp" "../libparticle/particle.h")

list(APPEND source ${source_graph})
list(APPEND source ${source_ui})

source_group("4DWorld" FILES ${source})
source_group("audioNodes" FILES ${source_audioNodes})
source_group("objects" FILES ${source_objects})
source_group("tests" FILES ${source_tests})
source_group("ui" FILES ${source_ui})
source_group("3rdparty\\macos" FILES ${source_macos})
source_group("3rdparty\\leap" FILES ${source_leap})
source_group("3rdparty\\xmm" FILES ${source_xmm})
source_group("3rdparty\\delaunay" FILES ${source_delaunay})

add_executable(avgraph ${source})

target_compile_definitions(avgraph PRIVATE NO_SDL_GLEXT)

target_compile_definitions(avgraph PRIVATE NO_SDL_GLEXT ENABLE_KINECT1=0 ENABLE_KINECT2=0 ENABLE_TURBOJPEG=0 ENABLE_DEEPBELIEF=0 ENABLE_AVCODEC=0)

if (WIN32)
	target_compile_definitions(avgraph PRIVATE HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX)
endif (WIN32)

target_include_directories(avgraph PRIVATE "${PROJECT_SOURCE_DIR}")
target_include_directories(avgraph PRIVATE "objects")

if (APPLE)
	target_include_directories(avgraph PRIVATE "../avgraph")
	target_include_directories(avgraph PRIVATE "../avgraph/macos/include")
	target_include_directories(avgraph PRIVATE "../vfxpro/macos/include")
	target_include_directories(avgraph PRIVATE "../vfxpro/")
	target_include_directories(avgraph PRIVATE "../../../libfreenect2/macos/include")
endif (APPLE)

if (WIN32)
	target_include_directories(avgraph PRIVATE "../vfxpro/win32/include")
	target_include_directories(avgraph PRIVATE "../vfxpro/")
endif (WIN32)

target_link_libraries(avgraph PRIVATE framework)
target_link_libraries(avgraph PRIVATE libosc)
target_link_libraries(avgraph PRIVATE libgg)

if (MSVC)
	target_link_libraries(avgraph PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/FreeImage.lib)
	target_link_libraries(avgraph PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/freetype252.lib)
	target_link_libraries(avgraph PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/glew32.lib)
	target_link_libraries(avgraph PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/OpenAL32.lib)
	target_link_libraries(avgraph PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/SDL2.lib)
	target_link_libraries(avgraph PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/SDL2main.lib)
	target_link_libraries(avgraph PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/XInput9_1_0.lib)
	target_link_libraries(avgraph PRIVATE Winmm.lib OpenGL32.lib WSock32.lib Ws2_32.lib)
endif (MSVC)

if (APPLE)
	target_compile_definitions(avgraph PUBLIC MACOS)

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
	
	target_link_libraries(avgraph PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/macos/glew/lib/libGLEW.a)
	target_link_libraries(avgraph PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/macos/lib/libfreeimage.a)
	target_link_libraries(avgraph PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/macos/lib/libfreetype.a)
	target_link_libraries(avgraph PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/macos/lib/libSDL2.a)
	target_link_libraries(avgraph PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/macos/lib/libSDL2main.a)
	target_link_libraries(avgraph PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../avpaint/macos/lib/libportaudio.a)
	target_link_libraries(avgraph PRIVATE ${COCOA_LIBRARY})
	target_link_libraries(avgraph PRIVATE ${CARBON_LIBRARY})
	target_link_libraries(avgraph PRIVATE ${COREAUDIO_LIBRARY})
	target_link_libraries(avgraph PRIVATE ${COREVIDEO_LIBRARY})
	target_link_libraries(avgraph PRIVATE ${IOKIT_LIBRARY})
	target_link_libraries(avgraph PRIVATE ${AUDIOTOOLBOX_LIBRARY})
	target_link_libraries(avgraph PRIVATE ${AUDIOUNIT_LIBRARY})
	target_link_libraries(avgraph PRIVATE ${OPENAL_LIBRARY})
	target_link_libraries(avgraph PRIVATE ${OPENGL_LIBRARY})
	target_link_libraries(avgraph PRIVATE ${FF_LIBRARY})
	target_link_libraries(avgraph PRIVATE ${ICONV_LIBRARY})
	target_link_libraries(avgraph PRIVATE ${BZ2_LIBRARY}) # FreeType
endif (APPLE)

if (MSVC)
	set_property(TARGET avgraph APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
	set_property(TARGET avgraph APPEND_STRING PROPERTY LINK_FLAGS /SAFESEH:NO)
endif (MSVC)
