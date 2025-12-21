# SDL2 for with CMake

This repository contains simple projects demonstrating the use of SDL2.

## Setup on with CMake
- `cd` into the root folder of the repository.
- Run `git submodule update --init --recursive` to clone the SDL2 repository into the `vendored/SDL2` folder.
- Run `cmake -B build -S .` to generate the build files.
- `cd build` into the build folder.
- Run `make` to build the projects.

## Projects
- `1-stamp`: Basic window that allows stamping images with the mouse.
  - To run, execute `./1-stamp` from the build folder.
- `2-sketchpad`: A mini sketchpad application that allows drawing with the mouse, resizing cursor and changing colors.
  - To run, execute `./2-sketchpad` from the build folder.
- `3-connect4`: A simple Connect 4 game with basic AI.
  - Uses alpha-beta pruning for the AI.
  - Multithreaded to keep the UI responsive during AI calculations.
  - Animated piece dropping effect.
  - To run, execute `./3-connect4` from the build folder.
- More to come...
