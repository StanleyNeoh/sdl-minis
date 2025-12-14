# SDL2 for windows with MinGW

## Setup on Windows with MinGW
- Download SDL2-devel-2.x.x-mingw.zip from the [SDL2 website](https://github.com/libsdl-org/SDL/releases/tag/release-2.32.10).
- Copy `x86_64-w64-mingw32` folder to your `vendored` folder and rename it to `SDL2`.

## Projects
- Each project folder contains a `Makefile` that you can use to build and run the project.
- To build and run a project, navigate to the project folder in your terminal and run:
  ```
  make run
  ```
- This will compile the project and execute the resulting binary.
- Make sure to have `mingw-w64` installed and added to your system's PATH to use `g++` and `make` commands.
