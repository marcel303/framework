cmake_minimum_required(VERSION 2.6)

set(HAS_LIBGG 0)
set(HAS_LIBGG_FREEIMAGE 0)
set(HAS_LIBAGG 0)
set(HAS_LIBCXIMAGE 0)
set(HAS_LIBIPHONE 0)
set(HAS_LIBKLODDER 0)
set(HAS_LIBTINYXML 0)

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../../libgg libgg)
endif (NOT HAS_LIBGG)

if (NOT HAS_LIBGG_FREEIMAGE)
	message("including libgg_freeimage")
	add_subdirectory(../../libgg_freeimage libgg_freeimage)
endif (NOT HAS_LIBGG_FREEIMAGE)

if (NOT HAS_LIBIPHONE)
	message("including libiphone")
	add_subdirectory(../../libiphone libiphone)
endif (NOT HAS_LIBIPHONE)

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

add_executable(klodder ${source})

target_compile_definitions(klodder PRIVATE NO_SDL_GLEXT HAVE_M_PI _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX)

target_include_directories(klodder PRIVATE "${PROJECT_SOURCE_DIR}")
target_include_directories(klodder PRIVATE "../../users/marcel/framework/win32/include")

target_link_libraries(klodder PRIVATE libgg)
target_link_libraries(klodder PRIVATE libgg_freeimage)
target_link_libraries(klodder PRIVATE libagg)
target_link_libraries(klodder PRIVATE libcximage)
target_link_libraries(klodder PRIVATE libiphone)
target_link_libraries(klodder PRIVATE libklodder)
target_link_libraries(klodder PRIVATE libtinyxml)

target_link_libraries(klodder PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../users/marcel/framework/win32/lib/FreeImage.lib)
target_link_libraries(klodder PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../users/marcel/framework/win32/lib/SDL.lib)
target_link_libraries(klodder PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../users/marcel/framework/win32/lib/SDLmain.lib)
target_link_libraries(klodder PRIVATE Winmm.lib)

if (MSVC)
	set_property(TARGET klodder APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)

# --- klodder_ios ---

#set(CMAKE_OSX_SYSROOT iphoneos4.3)
#set(CMAKE_OSX_ARCHITECTURES "armv6;armv7;i386")
#set(CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphoneos;-iphonesimulator")
#
#file(GLOB source_ios RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cpp" "*.h" "*.mm" "*.m")
#file(GLOB resource_ios RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.xib" "../*.xib")
#
#add_executable(klodder_ios MACOSX_BUNDLE ${source_ios} ${resource_ios})
#
#target_include_directories(klodder_ios PRIVATE "${PROJECT_SOURCE_DIR}")
#
#target_link_libraries(klodder_ios PRIVATE libgg)
#target_link_libraries(klodder_ios PRIVATE libgg_freeimage)
#target_link_libraries(klodder_ios PRIVATE libagg)
#target_link_libraries(klodder_ios PRIVATE libcximage)
#target_link_libraries(klodder_ios PRIVATE libiphone)
#target_link_libraries(klodder_ios PRIVATE libklodder)
#target_link_libraries(klodder_ios PRIVATE libtinyxml)
#
#target_link_libraries(klodder_ios PRIVATE "-framework CoreGraphics" "-framework Foundation" "-framework UIKit")
#
#set_target_properties(klodder_ios PROPERTIES
#	MACOSX_BUNDLE_GUI_IDENTIFIER "com.granniesgames.klodder"
#	MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../klodder-Info.plist
#	RESOURCE "${resource_ios}"
#	XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
#	XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym"
#	XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER YES
#	XCODE_ATTRIBUTE_GCC_PREFIX_HEADER ${CMAKE_CURRENT_LIST_DIR}/klodder.pch
#	XCODE_ATTRIBUTE_INFOPLIST_PREPROCESS NO
#	XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET 3.0
#	)
