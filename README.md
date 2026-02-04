# Morphological Operations Demo

Interactive visualization of morphological image processing operations implemented in C++ with SDL2 and Dear ImGui.

## Overview

This project provides step-by-step animations of fundamental morphological operations used in binary image processing. It includes two separate demos:

- **Morphology Demo**: Erosion, dilation, and boundary detection
- **Flood Fill Demo**: Safe zone detection with configurable safety radius

## Building

### Requirements

- CMake 3.16 or later
- C++17 compatible compiler
- SDL2

### macOS

```bash
brew install sdl2
mkdir build && cd build
cmake ..
make -j4
```

### Linux (Ubuntu/Debian)

```bash
sudo apt-get install libsdl2-dev
mkdir build && cd build
cmake ..
make -j4
```

## Morphology Demo

Demonstrates erosion, dilation, and edge detection on binary images.

```bash
./build/erosion_demo
```

### Supported Operations

| Operation | Description |
|-----------|-------------|
| Erosion | Shrinks foreground regions. A pixel remains set only if all neighbors are set. |
| Dilation | Expands foreground regions. A pixel is set if any neighbor is set. |
| Inner Boundary | Detects inner edges: `Original - Eroded` |
| Outer Boundary | Detects outer edges: `Dilated - Original` |
| Gradient | Full edge detection: `Dilated XOR Eroded` |

### Boundary Modes

Controls how pixels outside the image boundary are treated:

| Mode | Behavior |
|------|----------|
| Zero | Out-of-bounds pixels are treated as background |
| One | Out-of-bounds pixels are treated as foreground |
| Extend | Edge pixels are replicated |
| Wrap | Opposite edges are connected |

### Controls

| Key | Action |
|-----|--------|
| Space | Play/Pause animation |
| R | Reset with new random pattern |
| E | Switch to erosion |
| D | Switch to dilation |
| Esc | Quit |

## Flood Fill Demo

Demonstrates the flood fill algorithm with an optional safety radius check. When enabled, the algorithm only fills positions where a circle of the specified radius fits entirely within the fillable region. This is similar to how VR headsets determine safe play areas.

```bash
./build/floodfill_demo
```

### Features

- BFS and DFS traversal comparison
- 4-connected and 8-connected neighborhood options
- Configurable safety radius for safe zone detection
- Real-time circle preview on hover

### Controls

| Key | Action |
|-----|--------|
| Click | Start fill at position |
| Space | Play/Pause |
| S | Single step |
| R | New random pattern |
| Up/Down | Adjust safety radius |
| Esc | Quit |

## Project Structure

```
src/
├── main.cpp                 # Morphology demo entry point
├── main_floodfill.cpp       # Flood fill demo entry point
├── binary_image.hpp/cpp     # Binary image container with noise generation
├── erosion.hpp/cpp          # Morphological operations
├── visualizer.hpp/cpp       # Morphology visualizer
├── floodfill.hpp/cpp        # Flood fill algorithm
└── floodfill_visualizer.hpp/cpp  # Flood fill visualizer
```

## How Morphological Erosion Works

1. A structuring element (kernel) is positioned at each pixel
2. All pixels covered by the structuring element are checked
3. If all covered pixels are foreground, the center pixel remains foreground
4. If any covered pixel is background, the center pixel becomes background

```
Original (5x5):         Structuring Element (3x3):      After Erosion:
    # # # # #                   * * *                     . . . . .
    # # # # #                   * * *                     . # # # .
    # # # # #                   * * *                     . # # # .
    # # # # #                                             . # # # .
    # # # # #                                             . . . . .
```

Dilation is the dual operation: a pixel is set if any neighbor under the structuring element is set.

## License

MIT License
