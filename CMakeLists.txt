cmake_minimum_required(VERSION 3.10)
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

add_compile_definitions(BLOB_DIR=\"${CMAKE_SOURCE_DIR}/blobs/\")

# 1-stamp
add_executable(stamp 1-stamp/stamp.cpp)

# 2-sketchpad
add_executable(sketchpad 2-sketchpad/sketchpad.cpp)

# 3-connect4
add_executable(connect4 3-connect4/main.cpp)

# 4-snake
file(GLOB DRAGBOX_SOURCES "4-dragbox/lib/**/*.cpp")
add_executable(dragbox 4-dragbox/main.cpp ${DRAGBOX_SOURCES})
