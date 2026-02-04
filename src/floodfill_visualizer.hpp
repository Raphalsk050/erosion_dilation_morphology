#ifndef FLOODFILL_VISUALIZER_HPP
#define FLOODFILL_VISUALIZER_HPP

#include "binary_image.hpp"
#include "floodfill.hpp"
#include <SDL2/SDL.h>
#include <memory>
#include <functional>

// UI state for flood fill visualization
struct FloodFillControls {
    int grid_size = 24;
    int min_grid_size = 10;
    int max_grid_size = 50;
    
    // Pattern generation
    float noise_scale = 0.2f;
    float noise_threshold = 0.45f;
    int noise_seed = 42;
    
    // Algorithm settings
    int selected_connectivity = 0;  // 0 = 4-connected, 1 = 8-connected
    int selected_algorithm = 0;     // 0 = BFS, 1 = DFS
    
    // Safety radius for clearance checking
    int safety_radius = 2;
    bool show_safety_preview = true;
    
    // Animation timing
    int animation_speed = 30;
    
    // State
    bool needs_regenerate = false;
    bool needs_resize = false;
    bool fill_started = false;
    int start_x = -1;
    int start_y = -1;
    
    // Mouse tracking
    int hover_x = -1;
    int hover_y = -1;
};

// Interactive visualizer for flood fill with safety radius constraint
class FloodFillVisualizer {
public:
    FloodFillVisualizer(int pixel_size = 18, int gap = 1);
    ~FloodFillVisualizer();

    FloodFillVisualizer(const FloodFillVisualizer&) = delete;
    FloodFillVisualizer& operator=(const FloodFillVisualizer&) = delete;

    bool initialize(int image_width, int image_height);
    void run(std::function<BinaryImage(const FloodFillControls&)> createImageFunc);
    FloodFillControls& getControls() { return controls_; }

private:
    void renderImGui();
    void renderGrid();
    bool handleEvents();
    void resetFill();
    void startFillAt(int x, int y);
    void updateHoverPosition(int mouse_x, int mouse_y);

    SDL_Window* window_ = nullptr;
    SDL_GLContext gl_context_ = nullptr;

    int pixel_size_;
    int gap_;
    int image_width_ = 0;
    int image_height_ = 0;
    int window_width_ = 0;
    int window_height_ = 0;
    int grid_offset_x_ = 0;
    int grid_offset_y_ = 0;

    FloodFillControls controls_;
    std::unique_ptr<BinaryImage> source_image_;
    std::unique_ptr<FloodFill> floodfill_;
    
    bool paused_ = true;
    bool completed_ = false;
    int steps_count_ = 0;
    Uint32 last_step_time_ = 0;
    
    std::function<BinaryImage(const FloodFillControls&)> create_image_func_;
};

#endif
