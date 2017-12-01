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

if (APPLE)
	target_compile_definitions(aniplay PUBLIC MACOS)
	target_compile_definitions(example PUBLIC MACOS)
	target_compile_definitions(model PUBLIC MACOS)
	target_compile_definitions(sounds PUBLIC MACOS)
	target_compile_definitions(sprites PUBLIC MACOS)
endif (APPLE)

target_link_libraries(aniplay PRIVATE ${FRAMEWORK_LIBRARIES})
target_link_libraries(example PRIVATE ${FRAMEWORK_LIBRARIES})
target_link_libraries(model PRIVATE ${FRAMEWORK_LIBRARIES})
target_link_libraries(sounds PRIVATE ${FRAMEWORK_LIBRARIES})
target_link_libraries(sprites PRIVATE ${FRAMEWORK_LIBRARIES})

if (MSVC)
	set_property(TARGET libparticle_test APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
	set_property(TARGET libparticle_test APPEND_STRING PROPERTY LINK_FLAGS /SAFESEH:NO)
endif (MSVC)
