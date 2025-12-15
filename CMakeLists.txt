cmake_minimum_required(VERSION 3.10)
project(sdl-minis)

# Configure SDL2 options before adding subdirectory
set(SDL_SHARED OFF CACHE BOOL "Build shared SDL" FORCE)
set(SDL_STATIC ON CACHE BOOL "Build static SDL" FORCE)
set(SDL_TEST OFF CACHE BOOL "Build SDL test" FORCE)

# Add the vendored SDL library
add_subdirectory(vendored/SDL)

# SDL2 builds headers into specific directories in the build tree.
# We need to add these to the include path so that #include <SDL.h> works.
# (Standard SDL2 usage is often <SDL2/SDL.h>, but the existing code uses <SDL.h>)
set(SDL2_BUILD_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/vendored/SDL/include/SDL2")
set(SDL2_BUILD_CONFIG_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/vendored/SDL/include-config-$<LOWER_CASE:$<CONFIG>>/SDL2")

# --- 1-stamp ---
add_executable(stamp 1-stamp/stamp.cpp)
target_link_libraries(stamp PRIVATE SDL2::SDL2-static)
if(TARGET SDL2::SDL2main)
    target_link_libraries(stamp PRIVATE SDL2::SDL2main)
endif()
target_include_directories(stamp PRIVATE "${SDL2_BUILD_INCLUDE_DIR}" "${SDL2_BUILD_CONFIG_INCLUDE_DIR}")

# --- 2-sketchpad ---
add_executable(sketchpad 2-sketchpad/sketchpad.cpp)
target_include_directories(sketchpad PRIVATE 2-sketchpad)
target_link_libraries(sketchpad PRIVATE SDL2::SDL2-static)
if(TARGET SDL2::SDL2main)
    target_link_libraries(sketchpad PRIVATE SDL2::SDL2main)
endif()
target_include_directories(sketchpad PRIVATE "${SDL2_BUILD_INCLUDE_DIR}" "${SDL2_BUILD_CONFIG_INCLUDE_DIR}")
