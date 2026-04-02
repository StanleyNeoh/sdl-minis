cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Optional: Use OFF for strict standard C++

project(sdl-minis)

# Add the vendored SDL library
add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)
add_subdirectory(vendored/SDL_ttf EXCLUDE_FROM_ALL)

link_libraries(SDL2::SDL2)
if(TARGET SDL2::SDL2main)
    link_libraries(SDL2::SDL2main)
endif()

if (TARGET SDL2_ttf::SDL2_ttf)
    link_libraries(SDL2_ttf::SDL2_ttf)
endif()

# Add the vendored imgui library
include_directories("vendored/imgui")
include_directories("vendored/imgui/backends")
add_library(imgui_lib 
    vendored/imgui/imgui.cpp
    vendored/imgui/imgui_demo.cpp
    vendored/imgui/imgui_draw.cpp
    vendored/imgui/imgui_tables.cpp
    vendored/imgui/imgui_widgets.cpp
    vendored/imgui/backends/imgui_impl_sdl2.cpp
    vendored/imgui/backends/imgui_impl_sdlrenderer2.cpp
)
link_libraries(imgui_lib)

add_compile_definitions(BLOB_DIR=\"${CMAKE_SOURCE_DIR}/blobs/\")

# 1-stamp
add_executable(stamp 1-stamp/stamp.cpp)

# 2-sketchpad
add_executable(sketchpad 2-sketchpad/sketchpad.cpp)

# 3-connect4
add_executable(connect4 3-connect4/main.cpp)

# 4-dragbox
file(GLOB DRAGBOX_SOURCES "4-dragbox/lib/**/*.cpp")
add_executable(dragbox 4-dragbox/main.cpp ${DRAGBOX_SOURCES})

# 5-snake
add_executable(snake 5-snake/snake.cpp)

# 6-particlebox
add_executable(particlebox 6-particlebox/particlebox.cpp)