cmake_minimum_required(VERSION 3.10)
project(sdl-minis)

# Add the vendored SDL library
add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)

link_libraries(SDL2::SDL2)
if(TARGET SDL2::SDL2main)
    link_libraries(SDL2::SDL2main)
endif()

# 1-stamp
add_executable(stamp 1-stamp/stamp.cpp)
target_compile_definitions(stamp PRIVATE BLOB_DIR=\"${CMAKE_SOURCE_DIR}/blobs/\")

# 2-sketchpad
add_executable(sketchpad 2-sketchpad/sketchpad.cpp)

# 3-connect4
add_executable(connect4 3-connect4/main.cpp)