# SDL2 for with CMake

This repository contains simple projects demonstrating the use of SDL2.

## Setup with CMake
- `cd` into the root folder of the repository.
- Run `cmake -B build -S .` to generate the build files. Dependencies are fetched automatically via CMake FetchContent.
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
- `4-dragbox`: A simple drag-and-drop box for trying out SDL2 TTF
  - To run, execute `./4-dragbox` from the build folder.
- `5-snake`: Snake game using ImGui for menu screens
  - To run, execute `./5-snake` from the build folder.
- `6-particlebox`: Particle physics simulation with ImGui settings panel
  - Configurable particle count, physics steps, and radius
  - Arena-allocated quadtree for O(n log n) spatial collision queries instead of brute-force O(n²)
  - OpenMP parallelism for particle stepping, quadtree querying (thread-local buffers), and wall collision detection
  - Lock-free parallel collision resolution via greedy graph coloring — pairs are partitioned into conflict-free batches so no two pairs in a batch share a particle
  - Cache-friendly compact particle layout and chunked quadtree storage to reduce allocation overhead
  - To run, execute `./6-particlebox` from the build folder.
