/**
 * Morphology Demo
 *
 * Interactive visualization of erosion, dilation, and edge detection.
 */

#include "binary_image.hpp"
#include "erosion.hpp"
#include "visualizer.hpp"
#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>

BinaryImage createImageFromControls(const UIControls& controls) {
    int size = controls.grid_size;
    
    switch (controls.selected_shape) {
        case 0:
            return BinaryImage::createRectangle(size, size, 2);
        case 1:
            return BinaryImage::createCross(size, size, 3);
        case 2:
            return BinaryImage::createLShape(size, size);
        case 3:
            return BinaryImage::createCircle(size, size, size / 3);
        case 4:
        default:
            return BinaryImage::createNoise(size, size, 
                                            controls.noise_scale, 
                                            controls.noise_threshold, 
                                            controls.noise_seed);
    }
}

int main(int argc, char* argv[]) {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    std::cout << "Morphology Demo\n";
    std::cout << "---------------\n\n";
    std::cout << "Operations: Erosion, Dilation, Inner/Outer Boundary, Gradient\n";
    std::cout << "Use the control panel to configure parameters.\n\n";
    std::cout << "Controls:\n";
    std::cout << "  Space  - Play/Pause\n";
    std::cout << "  R      - Reset animation\n";
    std::cout << "  E/D    - Switch erosion/dilation\n";
    std::cout << "  Esc    - Quit\n\n";

    Visualizer visualizer(14, 1);
    
    UIControls& controls = visualizer.getControls();
    controls.grid_size = 20;
    controls.selected_shape = 4;
    
    if (!visualizer.initialize(controls.grid_size, controls.grid_size)) {
        std::cerr << "Failed to initialize visualizer\n";
        return 1;
    }

    visualizer.run(createImageFromControls);

    return 0;
}
