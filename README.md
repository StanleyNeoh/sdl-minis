# SDL2 for with CMake

This repository contains simple projects demonstrating the use of SDL2.

## Setup with CMake
- `cd` into the root folder of the repository.
- Run `cmake -B build -S .` to generate the build files. Dependencies are fetched automatically via CMake FetchContent.
- `cd build` into the build folder.
- Run `make` to build the projects.

## Build with Ninja
- Install Ninja and make sure `ninja` is available on your `PATH`.
- From the repository root, configure a Ninja build directory:
  - `cmake -S . -B build-ninja -G Ninja`
- Build all projects:
  - `cmake --build build-ninja`
- Build a single target:
  - `cmake --build build-ninja --target stamp`
- Run executables from the generated build directory, for example:
  - `./build-ninja/1-stamp/stamp`

If an existing build directory was generated with another CMake generator, such as NMake Makefiles, create a new build directory or delete that directory's `CMakeCache.txt` and `CMakeFiles` before switching it to Ninja.

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
  - Configurable particle count, radius range, mass range, gravity, and collision restitution (0–2, sub-elastic to super-elastic)
  - Per-particle mass with mass-weighted elastic collisions; hue-based coloring (blue=light, red=heavy)
  - Arena-allocated quadtree for O(n log n) spatial collision queries instead of brute-force O(n²)
  - Multiple multithreading modes selectable at runtime: graph coloring (lock-free), mutex locks, unsafe (no lock), and naive N²
  - Lock-free parallel collision resolution via greedy graph coloring — pairs are partitioned into conflict-free batches so no two pairs in a batch share a particle
  - Cache-friendly compact particle layout and chunked quadtree storage to reduce allocation overhead
  - To run, execute `./6-particlebox` from the build folder.
- `7-raytracer`: A raytracer with first person camera controls
  - Raytracing is spread across frames and multithreaded to keep frames responsive while rendering
  - To run, execute `./7-raytracer` from the build folder.
- `8-server`: A simple TCP server that supports multiple clients chatting with each other on the same local network.
  - Supports group chats
  - To run, execute `./8-server` from the build folder.
