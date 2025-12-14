# Sketchpad

A simple sketchpad application using SDL2. Click and drag the mouse to draw on the window.

As a learning exercise, this project is built with SDL_Surfaces instead of SDL_Textures to handle pixel manipulation directly to help better understand the SDL_Surface API.
This means window viewports are implemented manually instead of using SDL_Renderer. Viewports are built with C++14 compatible SFINAE templates with Curiously recurring Template Pattern (CRTP).

## Setup
1. Rune `make run` to build and run the sketchpad application. You can optionally provide 3 arguments: the output BMP filename, width, and height. For example:
   ```
   make run
   ```
## Features
- Compile time assembly of viewports using C++14 SFINAE templates and CRTP.
- Cursor size adjustment with scroll wheel
- Colour selection with left control panel

