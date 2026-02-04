/**
 * Flood Fill Demo
 *
 * Interactive visualization of the flood fill algorithm with support for:
 * - BFS and DFS traversal
 * - 4-connected and 8-connected neighborhoods
 * - Safety radius constraint for clearance-based filling
 */

#include "binary_image.hpp"
#include "floodfill_visualizer.hpp"
#include <iostream>
#include <cstdlib>
#include <ctime>

BinaryImage createImageFromControls(const FloodFillControls& controls) {
    return BinaryImage::createNoise(
        controls.grid_size, 
        controls.grid_size, 
        controls.noise_scale, 
        controls.noise_threshold, 
        controls.noise_seed
    );
}

int main(int argc, char* argv[]) {
    std::srand(std::time(nullptr));
    
    std::cout << "Flood Fill Demo\n";
    std::cout << "---------------\n\n";
    std::cout << "Click on the grid to start filling from that position.\n";
    std::cout << "Use the control panel to adjust algorithm and parameters.\n\n";
    std::cout << "Controls:\n";
    std::cout << "  Click     - Start fill\n";
    std::cout << "  Space     - Play/Pause\n";
    std::cout << "  S         - Single step\n";
    std::cout << "  R         - New pattern\n";
    std::cout << "  Up/Down   - Adjust safety radius\n";
    std::cout << "  Esc       - Quit\n\n";

    int grid_size = 24;
    
    if (argc > 1) {
        int arg_size = std::atoi(argv[1]);
        if (arg_size >= 10 && arg_size <= 50) {
            grid_size = arg_size;
        }
    }
    
    FloodFillVisualizer visualizer(18, 1);
    visualizer.getControls().grid_size = grid_size;
    
    if (!visualizer.initialize(grid_size, grid_size)) {
        std::cerr << "Failed to initialize visualizer\n";
        return 1;
    }
    
    visualizer.run(createImageFromControls);
    
    return 0;
}
