cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 14)

set(HAS_LIBGG 0)
set(HAS_LIBOSC 0)
set(HAS_FRAMEWORK 0)
set(HAS_LIBVIDEO 0)
set(HAS_VFXGRAPH 0)

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
if (NOT HAS_VFXGRAPH)
	message("including vfxgraph")
	add_subdirectory(../vfxGraph vfxgraph)
endif (NOT HAS_VFXGRAPH)

#

project(avgraph)

# --- avgraph ---

file(GLOB_RECURSE source "*.c" "*.cpp" "*.h" "*.hpp" "*.mm")

source_group("Source Files" FILES ${source})

add_executable(avgraph ${source})

set_target_properties(avgraph PROPERTIES LINK_FLAGS "-Wl,-all_load")

target_compile_definitions(avgraph PRIVATE
	NO_SDL_GLEXT
	ENABLE_KINECT1=0
	ENABLE_KINECT2=1
	ENABLE_TURBOJPEG=1
	ENABLE_LEAPMOTION=1)

target_link_libraries(avgraph PRIVATE libgg)
target_link_libraries(avgraph PRIVATE libosc)
target_link_libraries(avgraph PRIVATE framework)
target_link_libraries(avgraph PRIVATE libvideo)
target_link_libraries(avgraph PRIVATE vfxgraph)

target_link_libraries(avgraph PRIVATE ${FRAMEWORK_LIBRARIES})
target_link_libraries(avgraph PRIVATE ${LIBVIDEO_LIBRARIES})
target_link_libraries(avgraph PRIVATE ${VFXGRAPH_LIBRARIES})

target_include_directories(avgraph PRIVATE "${PROJECT_SOURCE_DIR}")

if (WIN32)
	target_include_directories(avgraph PRIVATE "../vfxGraph/win32/include")
	target_include_directories(avgraph PRIVATE "../libvideo/win32/include")
	target_include_directories(avgraph PRIVATE "../libvideo/")

	target_compile_definitions(avgraph PRIVATE ENABLE_DEEPBELIEF=0 __WINDOWS_MM__)
endif (WIN32)

if (APPLE)
	target_include_directories(avgraph PRIVATE "../vfxGraph/macos/include")
	target_include_directories(avgraph PRIVATE "../libvideo/macos/include")
	target_include_directories(avgraph PRIVATE "../libvideo/")
	target_include_directories(avgraph PRIVATE "../libfreenect2/macos/include")

	target_compile_definitions(avgraph PRIVATE ENABLE_DEEPBELIEF=1 __MACOSX_CORE__)
endif (APPLE)

if (WIN32)
	target_compile_definitions(avgraph PRIVATE HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX)
	
	# set the current working directory
	
	set_target_properties(avgraph PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data")
	
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

if (MSVC)
	set_property(TARGET avgraph APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
	set_property(TARGET avgraph APPEND_STRING PROPERTY LINK_FLAGS /SAFESEH:NO)
endif (MSVC)
