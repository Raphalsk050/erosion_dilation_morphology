#include "floodfill_visualizer.hpp"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <SDL2/SDL.h>
#if defined(__APPLE__)
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif
#include <iostream>
#include <cstdlib>
#include <cmath>

FloodFillVisualizer::FloodFillVisualizer(int pixel_size, int gap)
    : pixel_size_(pixel_size)
    , gap_(gap)
{
}

FloodFillVisualizer::~FloodFillVisualizer() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    if (gl_context_) {
        SDL_GL_DeleteContext(gl_context_);
    }
    if (window_) {
        SDL_DestroyWindow(window_);
    }
    SDL_Quit();
}

bool FloodFillVisualizer::initialize(int image_width, int image_height) {
    image_width_ = image_width;
    image_height_ = image_height;

    int cell_size = pixel_size_ + gap_;
    int panel_width = cell_size * image_width + gap_;
    int panel_height = cell_size * image_height + gap_;
    
    int control_panel_width = 300;
    window_width_ = control_panel_width + panel_width + gap_ * 4;
    window_height_ = std::max(panel_height + gap_ * 2 + 60, 700);
    
    grid_offset_x_ = control_panel_width + gap_ * 2;
    grid_offset_y_ = gap_ + 40;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    window_ = SDL_CreateWindow(
        "Flood Fill - Safe Zone Detection",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        window_width_,
        window_height_,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI
    );

    if (!window_) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        return false;
    }

    gl_context_ = SDL_GL_CreateContext(window_);
    if (!gl_context_) {
        std::cerr << "OpenGL context creation failed: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_GL_MakeCurrent(window_, gl_context_);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;

    ImGui_ImplSDL2_InitForOpenGL(window_, gl_context_);
    ImGui_ImplOpenGL3_Init("#version 150");

    return true;
}

void FloodFillVisualizer::resetFill() {
    controls_.fill_started = false;
    controls_.start_x = -1;
    controls_.start_y = -1;
    completed_ = false;
    paused_ = true;
    steps_count_ = 0;
    
    Connectivity conn = controls_.selected_connectivity == 0 ? 
        Connectivity::Four : Connectivity::Eight;
    FillAlgorithm algo = controls_.selected_algorithm == 0 ? 
        FillAlgorithm::BFS : FillAlgorithm::DFS;
    floodfill_ = std::make_unique<FloodFill>(conn, algo, controls_.safety_radius);
}

void FloodFillVisualizer::startFillAt(int x, int y) {
    if (x < 0 || x >= image_width_ || y < 0 || y >= image_height_) {
        return;
    }
    
    controls_.fill_started = true;
    controls_.start_x = x;
    controls_.start_y = y;
    completed_ = false;
    paused_ = true;
    steps_count_ = 0;
    
    Connectivity conn = controls_.selected_connectivity == 0 ? 
        Connectivity::Four : Connectivity::Eight;
    FillAlgorithm algo = controls_.selected_algorithm == 0 ? 
        FillAlgorithm::BFS : FillAlgorithm::DFS;
    
    floodfill_ = std::make_unique<FloodFill>(conn, algo, controls_.safety_radius);
    floodfill_->initialize(*source_image_, x, y);
}

void FloodFillVisualizer::updateHoverPosition(int mouse_x, int mouse_y) {
    int cell_size = pixel_size_ + gap_;
    int grid_x = (mouse_x - grid_offset_x_ - gap_) / cell_size;
    int grid_y = (mouse_y - grid_offset_y_ - gap_) / cell_size;
    
    if (grid_x >= 0 && grid_x < image_width_ && 
        grid_y >= 0 && grid_y < image_height_) {
        controls_.hover_x = grid_x;
        controls_.hover_y = grid_y;
    } else {
        controls_.hover_x = -1;
        controls_.hover_y = -1;
    }
}

void FloodFillVisualizer::renderGrid() {
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    
    int cell_size = pixel_size_ + gap_;
    int panel_width = cell_size * image_width_ + gap_;
    
    // Header
    ImU32 title_color = IM_COL32(200, 200, 200, 255);
    char title[128];
    snprintf(title, sizeof(title), "SAFE ZONE (Radius=%d)", controls_.safety_radius);
    draw_list->AddText(ImVec2(grid_offset_x_ + panel_width / 2 - 70, grid_offset_y_ - 30), 
                       title_color, title);
    
    // Background
    float panel_x = static_cast<float>(grid_offset_x_);
    draw_list->AddRectFilled(
        ImVec2(panel_x, grid_offset_y_),
        ImVec2(panel_x + panel_width, grid_offset_y_ + cell_size * image_height_ + gap_),
        IM_COL32(20, 20, 25, 255)
    );
    
    // Build hover circle for preview
    std::vector<std::pair<int, int>> hover_circle;
    bool hover_safe = false;
    if (controls_.show_safety_preview && controls_.hover_x >= 0 && !controls_.fill_started) {
        int r = controls_.safety_radius;
        int r_sq = r * r;
        for (int dy = -r; dy <= r; ++dy) {
            for (int dx = -r; dx <= r; ++dx) {
                if (dx * dx + dy * dy <= r_sq) {
                    hover_circle.emplace_back(controls_.hover_x + dx, controls_.hover_y + dy);
                }
            }
        }
        
        // Check if all pixels in circle are foreground
        hover_safe = true;
        for (const auto& [px, py] : hover_circle) {
            if (px < 0 || px >= image_width_ || py < 0 || py >= image_height_ ||
                !source_image_->get(px, py)) {
                hover_safe = false;
                break;
            }
        }
    }
    
    // Draw each pixel
    for (int y = 0; y < image_height_; ++y) {
        for (int x = 0; x < image_width_; ++x) {
            float px = panel_x + gap_ + x * cell_size;
            float py = grid_offset_y_ + gap_ + y * cell_size;
            
            ImU32 color;
            bool is_foreground = source_image_->get(x, y);
            
            // Check if pixel is in hover circle
            bool in_hover = false;
            for (const auto& [hx, hy] : hover_circle) {
                if (hx == x && hy == y) {
                    in_hover = true;
                    break;
                }
            }
            
            if (!controls_.fill_started) {
                // Pre-fill state
                if (in_hover) {
                    color = hover_safe ? IM_COL32(100, 255, 100, 200) : IM_COL32(255, 100, 100, 200);
                } else if (is_foreground) {
                    color = IM_COL32(200, 200, 200, 255);
                } else {
                    color = IM_COL32(60, 40, 40, 255);
                }
            } else {
                // During fill
                PixelState state = floodfill_->getState(x, y);
                std::pair<int, int> current = floodfill_->getCurrentPixel();
                
                if (x == current.first && y == current.second) {
                    color = IM_COL32(255, 255, 50, 255);
                } else if (state == PixelState::InQueue) {
                    color = IM_COL32(255, 165, 0, 255);
                } else if (state == PixelState::Processed) {
                    color = IM_COL32(0, 200, 100, 255);
                } else if (state == PixelState::Unsafe) {
                    color = IM_COL32(255, 80, 80, 255);
                } else if (state == PixelState::Boundary) {
                    color = IM_COL32(80, 80, 150, 255);
                } else {
                    color = is_foreground ? IM_COL32(150, 150, 150, 255) : IM_COL32(60, 40, 40, 255);
                }
            }
            
            draw_list->AddRectFilled(
                ImVec2(px, py),
                ImVec2(px + pixel_size_, py + pixel_size_),
                color
            );
            
            // Mark start position
            if (controls_.fill_started && x == controls_.start_x && y == controls_.start_y) {
                draw_list->AddRect(
                    ImVec2(px - 1, py - 1),
                    ImVec2(px + pixel_size_ + 1, py + pixel_size_ + 1),
                    IM_COL32(255, 255, 0, 255), 0, 0, 2.0f
                );
            }
        }
    }
    
    // Border
    draw_list->AddRect(
        ImVec2(panel_x, grid_offset_y_),
        ImVec2(panel_x + panel_width, grid_offset_y_ + cell_size * image_height_ + gap_),
        IM_COL32(80, 80, 80, 255)
    );
}

void FloodFillVisualizer::renderImGui() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    renderGrid();

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(280, 680), ImGuiCond_FirstUseEver);
    
    ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_NoCollapse);
    
    ImGui::SeparatorText("About");
    ImGui::TextWrapped(
        "Flood fill with safety radius constraint. "
        "Only positions where a disk of radius R fits entirely within "
        "the fillable region are marked as safe."
    );
    
    // Safety radius
    ImGui::SeparatorText("Safety Radius");
    if (ImGui::SliderInt("Radius (R)", &controls_.safety_radius, 0, 5)) {
        if (controls_.fill_started) {
            startFillAt(controls_.start_x, controls_.start_y);
        }
    }
    ImGui::TextWrapped("R=0: Fill all reachable\nR>0: Require clearance");
    ImGui::Checkbox("Show radius preview", &controls_.show_safety_preview);
    
    // Algorithm
    ImGui::SeparatorText("Algorithm");
    const char* algorithms[] = {"BFS (Breadth-First)", "DFS (Depth-First)"};
    if (ImGui::Combo("Search", &controls_.selected_algorithm, algorithms, IM_ARRAYSIZE(algorithms))) {
        if (controls_.fill_started) {
            startFillAt(controls_.start_x, controls_.start_y);
        }
    }
    
    const char* connectivities[] = {"4-connected", "8-connected"};
    if (ImGui::Combo("Neighbors", &controls_.selected_connectivity, connectivities, IM_ARRAYSIZE(connectivities))) {
        if (controls_.fill_started) {
            startFillAt(controls_.start_x, controls_.start_y);
        }
    }
    
    // Grid
    ImGui::SeparatorText("Grid");
    if (ImGui::SliderInt("Size", &controls_.grid_size, controls_.min_grid_size, controls_.max_grid_size)) {
        controls_.needs_resize = true;
    }
    
    // Pattern
    ImGui::SeparatorText("Pattern");
    if (ImGui::SliderFloat("Density", &controls_.noise_threshold, 0.3f, 0.7f, "%.2f")) {
        controls_.needs_regenerate = true;
    }
    if (ImGui::SliderFloat("Scale", &controls_.noise_scale, 0.1f, 0.4f, "%.2f")) {
        controls_.needs_regenerate = true;
    }
    if (ImGui::Button("Regenerate")) {
        controls_.noise_seed = rand();
        controls_.needs_regenerate = true;
    }
    
    // Animation
    ImGui::SeparatorText("Animation");
    ImGui::SliderInt("Speed (ms)", &controls_.animation_speed, 5, 100);
    
    if (!controls_.fill_started) {
        ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "Click grid to start");
        if (controls_.hover_x >= 0) {
            ImGui::Text("Position: (%d, %d)", controls_.hover_x, controls_.hover_y);
        }
    } else {
        if (paused_) {
            if (ImGui::Button("Play", ImVec2(80, 30))) paused_ = false;
        } else {
            if (ImGui::Button("Pause", ImVec2(80, 30))) paused_ = true;
        }
        ImGui::SameLine();
        
        if (ImGui::Button("Step", ImVec2(80, 30))) {
            if (floodfill_ && !completed_) {
                if (!floodfill_->step()) completed_ = true;
                steps_count_++;
            }
        }
        ImGui::SameLine();
        
        if (ImGui::Button("Reset", ImVec2(80, 30))) {
            resetFill();
        }
        
        if (floodfill_) {
            ImGui::Text("Steps: %d", steps_count_);
            ImGui::Text("Safe: %zu | Unsafe: %zu", 
                floodfill_->getFilledCount(), floodfill_->getUnsafeCount());
            ImGui::Text("Queue: %zu", floodfill_->getFrontierSize());
            
            if (completed_) {
                ImGui::TextColored(ImVec4(0, 1, 0.4f, 1), "Done");
            }
        }
    }
    
    // Legend
    ImGui::SeparatorText("Legend");
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1), "Gray: Open area");
    ImGui::TextColored(ImVec4(0.4f, 0.25f, 0.25f, 1), "Dark: Obstacle");
    ImGui::TextColored(ImVec4(1, 1, 0.2f, 1), "Yellow: Current");
    ImGui::TextColored(ImVec4(1, 0.65f, 0, 1), "Orange: Frontier");
    ImGui::TextColored(ImVec4(0, 0.8f, 0.4f, 1), "Green: Safe zone");
    ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Red: Unsafe");
    
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool FloodFillVisualizer::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        
        switch (event.type) {
            case SDL_QUIT:
                return false;

            case SDL_MOUSEMOTION:
                if (!ImGui::GetIO().WantCaptureMouse) {
                    updateHoverPosition(event.motion.x, event.motion.y);
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    if (!ImGui::GetIO().WantCaptureMouse) {
                        int mx = event.button.x;
                        int my = event.button.y;
                        int cell_size = pixel_size_ + gap_;
                        
                        int grid_x = (mx - grid_offset_x_ - gap_) / cell_size;
                        int grid_y = (my - grid_offset_y_ - gap_) / cell_size;
                        
                        if (grid_x >= 0 && grid_x < image_width_ && 
                            grid_y >= 0 && grid_y < image_height_) {
                            startFillAt(grid_x, grid_y);
                        }
                    }
                }
                break;

            case SDL_KEYDOWN:
                if (!ImGui::GetIO().WantCaptureKeyboard) {
                    switch (event.key.keysym.sym) {
                        case SDLK_ESCAPE:
                        case SDLK_q:
                            return false;
                        case SDLK_SPACE:
                            if (controls_.fill_started) paused_ = !paused_;
                            break;
                        case SDLK_r:
                            controls_.needs_regenerate = true;
                            break;
                        case SDLK_s:
                            if (floodfill_ && !completed_ && controls_.fill_started) {
                                if (!floodfill_->step()) completed_ = true;
                                steps_count_++;
                            }
                            break;
                        case SDLK_UP:
                            controls_.safety_radius = std::min(5, controls_.safety_radius + 1);
                            if (controls_.fill_started) {
                                startFillAt(controls_.start_x, controls_.start_y);
                            }
                            break;
                        case SDLK_DOWN:
                            controls_.safety_radius = std::max(0, controls_.safety_radius - 1);
                            if (controls_.fill_started) {
                                startFillAt(controls_.start_x, controls_.start_y);
                            }
                            break;
                        default:
                            break;
                    }
                }
                break;
            default:
                break;
        }
    }
    return true;
}

void FloodFillVisualizer::run(std::function<BinaryImage(const FloodFillControls&)> createImageFunc) {
    create_image_func_ = createImageFunc;
    source_image_ = std::make_unique<BinaryImage>(createImageFunc(controls_));
    
    Connectivity conn = controls_.selected_connectivity == 0 ? 
        Connectivity::Four : Connectivity::Eight;
    FillAlgorithm algo = controls_.selected_algorithm == 0 ? 
        FillAlgorithm::BFS : FillAlgorithm::DFS;
    floodfill_ = std::make_unique<FloodFill>(conn, algo, controls_.safety_radius);

    bool running = true;
    while (running) {
        running = handleEvents();
        
        if (controls_.needs_resize) {
            controls_.needs_resize = false;
            image_width_ = controls_.grid_size;
            image_height_ = controls_.grid_size;
            
            int cell_size = pixel_size_ + gap_;
            int panel_width = cell_size * image_width_ + gap_;
            int panel_height = cell_size * image_height_ + gap_;
            int control_panel_width = 300;
            window_width_ = control_panel_width + panel_width + gap_ * 4;
            window_height_ = std::max(panel_height + gap_ * 2 + 60, 700);
            
            SDL_SetWindowSize(window_, window_width_, window_height_);
            controls_.needs_regenerate = true;
        }
        
        if (controls_.needs_regenerate) {
            controls_.needs_regenerate = false;
            source_image_ = std::make_unique<BinaryImage>(create_image_func_(controls_));
            image_width_ = source_image_->width();
            image_height_ = source_image_->height();
            resetFill();
        }
        
        // Animate
        if (!paused_ && !completed_ && controls_.fill_started && floodfill_) {
            Uint32 current_time = SDL_GetTicks();
            if (current_time - last_step_time_ >= static_cast<Uint32>(controls_.animation_speed)) {
                last_step_time_ = current_time;
                if (!floodfill_->step()) completed_ = true;
                steps_count_++;
            }
        }

        int display_w, display_h;
        SDL_GL_GetDrawableSize(window_, &display_w, &display_h);
        
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        renderImGui();
        
        SDL_GL_SwapWindow(window_);
        SDL_Delay(10);
    }
}
