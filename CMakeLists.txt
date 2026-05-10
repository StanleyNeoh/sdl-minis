cmake_minimum_required(VERSION 3.14)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Optional: Use OFF for strict standard C++

project(sdl-minis)

include(FetchContent)

# SDL2
FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.32.10
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/vendored/SDL
)
FetchContent_MakeAvailable(SDL2)

# SDL2_ttf
FetchContent_Declare(
    SDL2_ttf
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
    GIT_TAG release-2.24.0
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/vendored/SDL_ttf
)
FetchContent_MakeAvailable(SDL2_ttf)

# imgui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.5
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/vendored/imgui
)
FetchContent_Populate(imgui)

# GoogleTest
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/vendored/googletest
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

link_libraries(SDL2::SDL2)
if(TARGET SDL2::SDL2main)
    link_libraries(SDL2::SDL2main)
endif()

if (TARGET SDL2_ttf::SDL2_ttf)
    link_libraries(SDL2_ttf::SDL2_ttf)
endif()

# Add the vendored imgui library
include_directories("${imgui_SOURCE_DIR}")
include_directories("${imgui_SOURCE_DIR}/backends")
add_library(imgui_lib 
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.cpp
)
target_link_libraries(imgui_lib PRIVATE SDL2::SDL2)
link_libraries(imgui_lib)

add_compile_definitions(BLOB_DIR=\"${CMAKE_SOURCE_DIR}/blobs/\")

# Projects
add_subdirectory(1-stamp)
add_subdirectory(2-sketchpad)
add_subdirectory(3-connect4)
add_subdirectory(4-dragbox)
add_subdirectory(5-snake)
add_subdirectory(6-particlebox)
add_subdirectory(7-raytracer)
add_subdirectory(8-server)