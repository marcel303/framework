cmake_minimum_required(VERSION 2.6)

# --- dependencies ---

add_subdirectory(../../../libnet libnet)

project(replication1)

# --- replication1 ---

file(GLOB_RECURSE source "*.cpp" "*.h" "*.inl")
list(REMOVE_ITEM source "${PROJECT_SOURCE_DIR}/Graphics/GraphicsDeviceD3D9.cpp")
list(REMOVE_ITEM source "${PROJECT_SOURCE_DIR}/Graphics/GraphicsDeviceD3D11.cpp")

source_group(Resources, Res.*)
source_group(Entities, Entity.*)

add_executable(replication1 ${source})

target_compile_definitions(replication1 PUBLIC SYSTEM_WINDOWS)
target_compile_definitions(replication1 PRIVATE _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS _CRT_SECURE_NO_DEPRECATE)

target_include_directories(replication1 PRIVATE "${PROJECT_SOURCE_DIR}")
target_include_directories(replication1 PRIVATE "${PROJECT_SOURCE_DIR}/Event")
target_include_directories(replication1 PRIVATE "${PROJECT_SOURCE_DIR}/Game")
target_include_directories(replication1 PRIVATE "${PROJECT_SOURCE_DIR}/Graphics")
target_include_directories(replication1 PRIVATE "${PROJECT_SOURCE_DIR}/Res")
target_include_directories(replication1 PRIVATE "${PROJECT_SOURCE_DIR}/Sound")
target_include_directories(replication1 PRIVATE "${PROJECT_SOURCE_DIR}/Util")

target_link_libraries(replication1 PRIVATE libnet)
target_link_libraries(replication1 PRIVATE SDL_mixer.lib SDL.lib SDLmain.lib DevIL.lib OpenAL32.lib cg.lib cgGL.lib OpenGL32.lib WSock32.lib)
