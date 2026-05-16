cmake_minimum_required(VERSION 3.14)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

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

# Add the vendored imgui library
include_directories("${imgui_SOURCE_DIR}")
include_directories("${imgui_SOURCE_DIR}/backends")
add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.cpp
)
target_link_libraries(imgui_lib PRIVATE SDL2::SDL2)

set(SDL_MINIS_COMMON_LIBS SDL2::SDL2 imgui_lib)

if(TARGET SDL2_ttf::SDL2_ttf)
    list(APPEND SDL_MINIS_COMMON_LIBS SDL2_ttf::SDL2_ttf)
endif()

function(sdl_minis_link_common target_name)
    if(TARGET ${target_name})
        target_link_libraries(${target_name} PRIVATE ${SDL_MINIS_COMMON_LIBS})
    endif()
endfunction()

add_compile_definitions(BLOB_DIR=\"${CMAKE_SOURCE_DIR}/blobs/\" SDL_MAIN_HANDLED)

# Projects
add_subdirectory(1-stamp)
add_subdirectory(2-sketchpad)
add_subdirectory(3-connect4)
add_subdirectory(4-dragbox)
add_subdirectory(5-snake)
add_subdirectory(6-particlebox)
add_subdirectory(7-raytracer)
add_subdirectory(8-server)

sdl_minis_link_common(stamp)
sdl_minis_link_common(sketchpad)
sdl_minis_link_common(connect4)
sdl_minis_link_common(dragbox)
sdl_minis_link_common(snake)
sdl_minis_link_common(particlebox)
sdl_minis_link_common(raytracer)
sdl_minis_link_common(tcp)
sdl_minis_link_common(client)