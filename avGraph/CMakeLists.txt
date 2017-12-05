cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 14)

set(HAS_AVGRAPH 1 PARENT_SCOPE)
message("setting HAS_AVGRAPH")

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../libgg libgg)
endif (NOT HAS_LIBGG)
if (NOT HAS_FRAMEWORK)
	message("including framework")
	add_subdirectory(../framework framework)
endif (NOT HAS_FRAMEWORK)

#

project(avGraph)

# --- avGraph ---

file(GLOB_RECURSE source "*.cpp" "*.h")

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

source_group("avGraph" FILES ${source})
source_group("ui" FILES ${source_ui})

add_library(avGraph ${source})

target_link_libraries(avGraph framework)
target_link_libraries(avGraph libgg)

target_include_directories(avGraph PUBLIC "${PROJECT_SOURCE_DIR}")

if (MSVC)
	set_property(TARGET avGraph APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
	set_property(TARGET avGraph APPEND_STRING PROPERTY LINK_FLAGS /SAFESEH:NO)
endif (MSVC)
