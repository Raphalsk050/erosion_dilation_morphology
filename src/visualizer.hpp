#ifndef VISUALIZER_HPP
#define VISUALIZER_HPP

#include "binary_image.hpp"
#include "erosion.hpp"
#include <SDL2/SDL.h>
#include <memory>
#include <string>
#include <functional>

/**
 * @brief Animation state for step-by-step morphology visualization.
 */
struct AnimationState {
    int current_x = 0;
    int current_y = 0;
    bool paused = true;
    bool completed = false;
    int speed_ms = 50;
    Uint32 last_step_time = 0;
};

/**
 * @brief UI control parameters exposed to ImGui.
 */
struct UIControls {
    // Grid size
    int grid_size = 16;
    int min_grid_size = 8;
    int max_grid_size = 32;
    
    // Noise parameters
    float noise_scale = 0.2f;
    float noise_threshold = 0.45f;
    int noise_seed = 42;
    
    // Structuring element
    int se_size = 3;
    bool se_is_cross = false;
    
    // Animation speed
    int animation_speed = 50;
    
    // Shape selection
    int selected_shape = 4;  // 0=rect, 1=cross, 2=lshape, 3=circle, 4=noise
    
    // Morphological operation selection
    int selected_operation = 0;  // 0=erosion, 1=dilation
    
    // Boundary mode selection
    int selected_boundary = 0;  // 0=zero, 1=one, 2=extend, 3=wrap
    
    // Flags
    bool needs_regenerate = false;
    bool needs_resize = false;
};

/**
 * @brief SDL2+ImGui visualizer for morphological operations animation.
 */
class Visualizer {
public:
    Visualizer(int pixel_size = 16, int gap = 1);
    ~Visualizer();

    Visualizer(const Visualizer&) = delete;
    Visualizer& operator=(const Visualizer&) = delete;

    bool initialize(int image_width, int image_height);
    void run(std::function<BinaryImage(const UIControls&)> createImageFunc);
    UIControls& getControls() { return controls_; }

private:
    void renderImGui(const BinaryImage& original, BinaryImage& result);
    void renderGrids(const BinaryImage& original, 
                     const BinaryImage& result,
                     const Morphology& morph);
    void drawPixelGL(float x, float y, float size, float r, float g, float b);

    void stepAnimation(AnimationState& state, 
                       const BinaryImage& original,
                       BinaryImage& result,
                       const Morphology& morph);
    void resetAnimation();
    bool handleEvents();

    // SDL/OpenGL resources
    SDL_Window* window_ = nullptr;
    SDL_GLContext gl_context_ = nullptr;

    // Display parameters
    int pixel_size_;
    int gap_;
    int image_width_ = 0;
    int image_height_ = 0;
    int window_width_ = 0;
    int window_height_ = 0;
    int grid_offset_x_ = 0;
    int grid_offset_y_ = 0;

    // State
    AnimationState anim_state_;
    UIControls controls_;
    
    // Current images and morphology processor
    std::unique_ptr<BinaryImage> current_image_;
    std::unique_ptr<BinaryImage> result_image_;
    std::unique_ptr<Morphology> morphology_;
};

#endif // VISUALIZER_HPP
