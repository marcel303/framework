cmake_minimum_required(VERSION 2.6)

# --- dependencies ---

add_subdirectory(../../../libgg libgg)
add_subdirectory(../../../libnet libnet)
add_subdirectory(../framework framework)

project(netarena)

# --- netarena ---

file(GLOB source "*.cpp" "*.h")

add_executable(netarena ${source})

target_compile_definitions(netarena PRIVATE _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS _CRT_SECURE_NO_DEPRECATE)

target_include_directories(netarena PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(netarena PRIVATE framework)
target_link_libraries(netarena PRIVATE libnet)
target_link_libraries(netarena PRIVATE libgg)

target_link_libraries(netarena PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/FreeImage.lib)
target_link_libraries(netarena PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/freetype252.lib)
target_link_libraries(netarena PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/glew32.lib)
target_link_libraries(netarena PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/OpenAL32.lib)
target_link_libraries(netarena PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/SDL2.lib)
target_link_libraries(netarena PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/SDL2main.lib)
target_link_libraries(netarena PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../framework/win32/lib/XInput9_1_0.lib)
target_link_libraries(netarena PRIVATE OpenGL32.lib WSock32.lib)
