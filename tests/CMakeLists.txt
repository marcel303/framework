cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 14)

set(HAS_LIBGG 0)
set(HAS_LIBOSC 0)
set(HAS_FRAMEWORK 0)
set(HAS_LIBVIDEO 0)

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../libgg libgg)
endif (NOT HAS_LIBGG)
if (NOT HAS_LIBOSC)
	message("including libosc")
	add_subdirectory(../libosc libosc)
endif (NOT HAS_LIBOSC)
if (NOT HAS_FRAMEWORK)
	message("including framework")
	add_subdirectory(../framework framework)
endif (NOT HAS_FRAMEWORK)
if (NOT HAS_LIBVIDEO)
	message("including libvideo")
	add_subdirectory(../libvideo libvideo)
endif (NOT HAS_LIBVIDEO)

#

project(tests)

# --- tests ---

file(GLOB_RECURSE source "*.c" "*.cpp" "*.h" "*.hpp" "*.mm")

# todo : link against vfxGraph as a library, not including stuff manually like we do here
file(GLOB_RECURSE source_vfxGraph
	"../vfxGraph/*.c"
	"../vfxGraph/*.cpp"
	"../vfxGraph/*.h"
	"../vfxGraph/*.hpp"
	"../vfxGraph/*.mm")

file(GLOB_RECURSE source_datgui "ofxDatGui/*.*")
file(GLOB_RECURSE source_nanovg "nanovg/*.*")

file(GLOB source_tests "test*.*")

file(GLOB_RECURSE source_ui
	"../libparticle/ui.cpp"
	"../libparticle/ui.h"
	"../libparticle/textfield.cpp"
	"../libparticle/textfield.h"
	"../libparticle/colorwheel.cpp"
	"../libparticle/colorwheel.h"
	"../libparticle/particle.cpp"
	"../libparticle/particle.h")

list(APPEND source ${source_ui})
list(APPEND source ${source_vfxGraph})
#list(REMOVE_ITEM source ${source_datgui})

source_group("Source Files" FILES ${source})
source_group("vfxGraph" FILES ${source_vfxGraph})
source_group("tests" FILES ${source_tests})
source_group("ui" FILES ${source_ui})
source_group("3rdparty\\ofxDatGui" FILES ${source_datgui})
source_group("3rdparty\\msdfgen" FILES ${source_msdfgen})
source_group("3rdparty\\nanovg" FILES ${source_nanovg})
source_group("3rdparty\\macos" FILES ${source_macos})
source_group("3rdparty\\leap" FILES ${source_leap})
source_group("3rdparty\\xmm" FILES ${source_xmm})
source_group("3rdparty\\rtmidi" FILES ${source_rtmidi})
source_group("3rdparty\\json" FILES ${source_json})
source_group("3rdparty\\ofxJpegGlitch" FILES ${source_oxfJpegGlitch})

if (WIN32)
	list(REMOVE_ITEM source ${source_xmm})
endif (WIN32)

add_executable(tests ${source})

target_compile_definitions(tests PRIVATE NO_SDL_GLEXT)

target_compile_definitions(tests PRIVATE NO_SDL_GLEXT ENABLE_KINECT1=0 ENABLE_KINECT2=1 ENABLE_TURBOJPEG=1 ENABLE_LEAPMOTION=1)

if (WIN32)
	target_compile_definitions(tests PRIVATE HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX)
	
	# set the current working directory
	
	set_target_properties(tests PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data")
	
	# copy DLL files to the Debug and Release folders
	
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/dll/FreeImage.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/dll/glew32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/dll/OpenAL32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/dll/SDL2.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/dll/XInput9_1_0.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../vfxGraph/win32/dll/portaudio_x86.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../vfxGraph/win32/dll/turbojpeg.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../libvideo/win32/dll/avcodec-57.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../libvideo/win32/dll/avdevice-57.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../libvideo/win32/dll/avfilter-6.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../libvideo/win32/dll/avformat-57.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../libvideo/win32/dll/avutil-55.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../libvideo/win32/dll/postproc-54.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../libvideo/win32/dll/swresample-2.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../libvideo/win32/dll/swscale-4.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/dll/FreeImage.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/dll/glew32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/dll/OpenAL32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/dll/SDL2.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/dll/XInput9_1_0.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../vfxGraph/win32/dll/portaudio_x86.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../vfxGraph/win32/dll/turbojpeg.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../libvideo/win32/dll/avcodec-57.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../libvideo/win32/dll/avdevice-57.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../libvideo/win32/dll/avfilter-6.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../libvideo/win32/dll/avformat-57.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../libvideo/win32/dll/avutil-55.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../libvideo/win32/dll/postproc-54.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../libvideo/win32/dll/swresample-2.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../libvideo/win32/dll/swscale-4.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
endif (WIN32)

if (APPLE)
	# copy dylib and framework files to the Debug and Release folders

	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../vfxGraph/macos/framework/Deepbelief.framework DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../vfxGraph/macos/lib/libLeap.dylib DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../vfxGraph/macos/lib/libturbojpeg.0.dylib DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug/)

	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../vfxGraph/macos/framework/Deepbelief.framework DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../vfxGraph/macos/lib/libLeap.dylib DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../vfxGraph/macos/lib/libturbojpeg.0.dylib DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Release/)
endif (APPLE)

target_include_directories(tests PRIVATE "${PROJECT_SOURCE_DIR}")

if (WIN32)
	target_include_directories(tests PRIVATE "../vfxGraph/macos/include")
	target_include_directories(tests PRIVATE "../libvideo/win32/include")
	target_include_directories(tests PRIVATE "../libvideo/")

	target_compile_definitions(tests PRIVATE ENABLE_DEEPBELIEF=0 __WINDOWS_MM__)
endif (WIN32)

if (APPLE)
	target_include_directories(tests PRIVATE "../vfxGraph")
	target_include_directories(tests PRIVATE "../vfxGraph/macos/include")
	target_include_directories(tests PRIVATE "../libvideo/macos/include")
	target_include_directories(tests PRIVATE "../libvideo/")
	target_include_directories(tests PRIVATE "../libfreenect2/macos/include")

	target_compile_definitions(tests PRIVATE ENABLE_DEEPBELIEF=1 __MACOSX_CORE__)
endif (APPLE)

target_link_libraries(tests PRIVATE framework)
target_link_libraries(tests PRIVATE libosc)
target_link_libraries(tests PRIVATE libgg)
target_link_libraries(tests PRIVATE libvideo)

target_link_libraries(tests PRIVATE ${FRAMEWORK_LIBRARIES})
target_link_libraries(tests PRIVATE ${LIBVIDEO_LIBRARIES})

if (MSVC)
	target_link_libraries(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/Leap.lib)
	target_link_libraries(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/portaudio_x86.lib)
	target_link_libraries(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/turbojpeg.lib)
endif (MSVC)

if (APPLE)
	target_compile_definitions(tests PUBLIC MACOS)

	find_library(COREMIDI_LIBRARY CoreMIDI)
	find_library(AVFOUNDATION_LIBRARY AVFoundation)

	target_link_libraries(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../libfreenect2/macos/lib/libfreenect2.0.2.0.dylib)
	target_link_libraries(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../vfxGraph//macos/lib/libLeap.dylib)
	target_link_libraries(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../vfxGraph//macos/lib/libportaudio.a)
	target_link_libraries(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../vfxGraph//macos/lib/libturbojpeg.0.dylib)
	target_link_libraries(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../vfxGraph//macos/framework/DeepBelief.framework)
	
	target_link_libraries(tests PRIVATE ${COREMIDI_LIBRARY})
	target_link_libraries(tests PRIVATE ${AVFOUNDATION_LIBRARY})
endif (APPLE)

if (MSVC)
	set_property(TARGET tests APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
	set_property(TARGET tests APPEND_STRING PROPERTY LINK_FLAGS /SAFESEH:NO)
endif (MSVC)
