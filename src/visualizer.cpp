#include "visualizer.hpp"
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

Visualizer::Visualizer(int pixel_size, int gap)
    : pixel_size_(pixel_size)
    , gap_(gap)
{
}

Visualizer::~Visualizer() {
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

bool Visualizer::initialize(int image_width, int image_height) {
    image_width_ = image_width;
    image_height_ = image_height;

    int cell_size = pixel_size_ + gap_;
    int panel_width = cell_size * image_width + gap_;
    int panel_height = cell_size * image_height + gap_;
    
    int control_panel_width = 280;
    window_width_ = control_panel_width + panel_width * 2 + gap_ * 4;
    window_height_ = std::max(panel_height + gap_ * 2 + 40, 600);
    
    grid_offset_x_ = control_panel_width + gap_ * 2;
    grid_offset_y_ = gap_ + 30;

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
        "Morphological Operations - Interactive Demo",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        window_width_,
        window_height_,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
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

void Visualizer::drawPixelGL(float x, float y, float size, float r, float g, float b) {
    // Not used - using ImGui DrawList
}

void Visualizer::renderGrids(const BinaryImage& original, 
                              const BinaryImage& result,
                              const Morphology& morph) {
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    
    std::vector<std::pair<int, int>> se_positions;
    if (!anim_state_.completed) {
        se_positions = morph.getCoveredPositions(anim_state_.current_x, anim_state_.current_y);
    }

    int cell_size = pixel_size_ + gap_;
    int panel_width = cell_size * image_width_ + gap_;
    
    // Determine operation type for coloring
    MorphOperation op = morph.getOperation();
    const char* op_labels[] = {"EROSION", "DILATION", "INNER EDGE", "OUTER EDGE", "GRADIENT"};
    const char* op_label = op_labels[static_cast<int>(op)];
    
    // Draw panel labels
    ImU32 label_color = IM_COL32(200, 200, 200, 255);
    draw_list->AddText(ImVec2(grid_offset_x_ + panel_width / 2 - 30, grid_offset_y_ - 25), 
                       label_color, "ORIGINAL");
    
    // Result label shows operation name
    char result_label[48];
    snprintf(result_label, sizeof(result_label), "RESULT (%s)", op_label);
    draw_list->AddText(ImVec2(grid_offset_x_ + panel_width + gap_ + panel_width / 2 - 50, grid_offset_y_ - 25), 
                       label_color, result_label);
    
    for (int panel = 0; panel < 2; ++panel) {
        float panel_x = static_cast<float>(grid_offset_x_ + panel * (panel_width + gap_));
        const BinaryImage& img = (panel == 0) ? original : result;
        
        draw_list->AddRectFilled(
            ImVec2(panel_x, grid_offset_y_),
            ImVec2(panel_x + panel_width, grid_offset_y_ + cell_size * image_height_ + gap_),
            IM_COL32(25, 25, 25, 255)
        );
        
        for (int y = 0; y < image_height_; ++y) {
            for (int x = 0; x < image_width_; ++x) {
                float px = panel_x + gap_ + x * cell_size;
                float py = grid_offset_y_ + gap_ + y * cell_size;
                
                ImU32 color;
                
                if (panel == 0) {
                    // Original panel with SE overlay
                    bool is_se_center = (x == anim_state_.current_x && y == anim_state_.current_y);
                    bool is_under_se = false;
                    
                    for (const auto& [sx, sy] : se_positions) {
                        if (sx == x && sy == y) {
                            is_under_se = true;
                            break;
                        }
                    }

                    if (!anim_state_.completed && is_se_center) {
                        color = IM_COL32(255, 50, 50, 255);  // Red - SE center
                    } else if (!anim_state_.completed && is_under_se) {
                        // Check if this position is out of bounds
                        int check_x = x;
                        int check_y = y;
                        bool oob = (check_x < 0 || check_x >= image_width_ || check_y < 0 || check_y >= image_height_);
                        // Actually, for the SE we need to check if the SE pixel would be OOB
                        // This happens when the current pixel is part of SE coverage
                        // For simplicity, just check if we're at a boundary
                        bool near_edge = (anim_state_.current_x < morphology_->getStructuringElement().center_x ||
                                          anim_state_.current_x >= image_width_ - morphology_->getStructuringElement().center_x ||
                                          anim_state_.current_y < morphology_->getStructuringElement().center_y ||
                                          anim_state_.current_y >= image_height_ - morphology_->getStructuringElement().center_y);
                        if (near_edge) {
                            color = IM_COL32(150, 100, 255, 255);  // Purple - near boundary
                        } else {
                            color = IM_COL32(255, 165, 0, 255);    // Orange - SE coverage
                        }
                    } else if (img.get(x, y)) {
                        color = IM_COL32(255, 255, 255, 255);  // White - foreground
                    } else {
                        color = IM_COL32(50, 50, 50, 255);     // Dark gray - background
                    }
                } else {
                    // Result panel
                    if (!anim_state_.completed && 
                        (y > anim_state_.current_y || 
                         (y == anim_state_.current_y && x >= anim_state_.current_x))) {
                        color = IM_COL32(35, 35, 35, 255);  // Not processed yet
                    } else if (img.get(x, y)) {
                        // Different colors based on operation
                        switch (op) {
                            case MorphOperation::Erosion:
                                color = IM_COL32(0, 230, 100, 255);    // Green - survived
                                break;
                            case MorphOperation::Dilation:
                                color = IM_COL32(100, 150, 255, 255);  // Blue - dilated
                                break;
                            case MorphOperation::InnerBoundary:
                            case MorphOperation::OuterBoundary:
                            case MorphOperation::Gradient:
                                color = IM_COL32(255, 200, 0, 255);    // Yellow - edge
                                break;
                        }
                    } else {
                        switch (op) {
                            case MorphOperation::Erosion:
                                color = IM_COL32(100, 50, 50, 255);    // Dark red - eroded
                                break;
                            case MorphOperation::Dilation:
                                color = IM_COL32(40, 40, 50, 255);     // Dark blue
                                break;
                            case MorphOperation::InnerBoundary:
                            case MorphOperation::OuterBoundary:
                            case MorphOperation::Gradient:
                                color = IM_COL32(35, 35, 35, 255);     // Dark - not edge
                                break;
                        }
                    }
                }
                
                draw_list->AddRectFilled(
                    ImVec2(px, py),
                    ImVec2(px + pixel_size_, py + pixel_size_),
                    color
                );
            }
        }
        
        draw_list->AddRect(
            ImVec2(panel_x, grid_offset_y_),
            ImVec2(panel_x + panel_width, grid_offset_y_ + cell_size * image_height_ + gap_),
            IM_COL32(80, 80, 80, 255)
        );
    }
}

void Visualizer::renderImGui(const BinaryImage& original, BinaryImage& result) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    renderGrids(original, *result_image_, *morphology_);

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(260, 580), ImGuiCond_FirstUseEver);
    
    ImGui::Begin("Morphology Controls", nullptr, ImGuiWindowFlags_NoCollapse);
    
    // Operation selection
    ImGui::SeparatorText("Operation");
    const char* operations[] = {"Erosion", "Dilation", "Inner Boundary", "Outer Boundary", "Gradient"};
    if (ImGui::Combo("Type", &controls_.selected_operation, operations, IM_ARRAYSIZE(operations))) {
        controls_.needs_regenerate = true;
    }
    // Show operation description
    const char* op_desc[] = {
        "Shrinks foreground (ALL neighbors = 1)",
        "Expands foreground (ANY neighbor = 1)",
        "Original - Eroded (internal edge)",
        "Dilated - Original (external edge)",
        "Dilated XOR Eroded (full edge)"
    };
    ImGui::TextWrapped("%s", op_desc[controls_.selected_operation]);
    
    // Boundary mode
    ImGui::SeparatorText("Boundary Mode");
    const char* boundaries[] = {"Zero (0)", "One (1)", "Extend", "Wrap"};
    if (ImGui::Combo("Mode", &controls_.selected_boundary, boundaries, IM_ARRAYSIZE(boundaries))) {
        controls_.needs_regenerate = true;
    }
    ImGui::TextWrapped("How out-of-bounds pixels are handled");
    
    // Shape selection
    ImGui::SeparatorText("Shape");
    const char* shapes[] = {"Rectangle", "Cross", "L-Shape", "Circle", "Noise"};
    if (ImGui::Combo("Shape", &controls_.selected_shape, shapes, IM_ARRAYSIZE(shapes))) {
        controls_.needs_regenerate = true;
    }
    
    // Grid size
    ImGui::SeparatorText("Grid Size");
    if (ImGui::SliderInt("Size", &controls_.grid_size, controls_.min_grid_size, controls_.max_grid_size)) {
        controls_.needs_resize = true;
    }
    if (ImGui::Button("Apply Size")) {
        controls_.needs_resize = true;
    }
    
    // Noise parameters
    if (controls_.selected_shape == 4) {
        ImGui::SeparatorText("Noise Parameters");
        if (ImGui::SliderFloat("Scale", &controls_.noise_scale, 0.05f, 0.5f, "%.2f")) {
            controls_.needs_regenerate = true;
        }
        if (ImGui::SliderFloat("Threshold", &controls_.noise_threshold, 0.2f, 0.8f, "%.2f")) {
            controls_.needs_regenerate = true;
        }
        if (ImGui::InputInt("Seed", &controls_.noise_seed)) {
            controls_.needs_regenerate = true;
        }
        if (ImGui::Button("Random Seed")) {
            controls_.noise_seed = rand();
            controls_.needs_regenerate = true;
        }
    }
    
    // Structuring element
    ImGui::SeparatorText("Structuring Element");
    if (ImGui::SliderInt("SE Size", &controls_.se_size, 3, 7)) {
        if (controls_.se_size % 2 == 0) controls_.se_size++;
        controls_.needs_regenerate = true;
    }
    if (ImGui::Checkbox("Cross Shape SE", &controls_.se_is_cross)) {
        controls_.needs_regenerate = true;
    }
    
    // Animation controls
    ImGui::SeparatorText("Animation");
    ImGui::SliderInt("Speed (ms)", &controls_.animation_speed, 5, 200);
    anim_state_.speed_ms = controls_.animation_speed;
    
    if (anim_state_.paused) {
        if (ImGui::Button("Play", ImVec2(80, 30))) {
            anim_state_.paused = false;
        }
    } else {
        if (ImGui::Button("Pause", ImVec2(80, 30))) {
            anim_state_.paused = true;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset", ImVec2(80, 30))) {
        controls_.needs_regenerate = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Step", ImVec2(80, 30))) {
        if (!anim_state_.completed) {
            anim_state_.last_step_time = 0;
            Uint32 saved_speed = anim_state_.speed_ms;
            anim_state_.speed_ms = 0;
            anim_state_.paused = false;
            stepAnimation(anim_state_, *current_image_, *result_image_, *morphology_);
            anim_state_.speed_ms = saved_speed;
            anim_state_.paused = true;
        }
    }
    
    // Progress
    float progress = static_cast<float>(anim_state_.current_y * image_width_ + anim_state_.current_x) /
                     static_cast<float>(image_width_ * image_height_);
    if (anim_state_.completed) progress = 1.0f;
    ImGui::ProgressBar(progress, ImVec2(-1, 0), anim_state_.completed ? "Complete!" : "");
    
    // Info
    ImGui::SeparatorText("Info");
    ImGui::Text("Grid: %dx%d", image_width_, image_height_);
    ImGui::Text("Position: (%d, %d)", anim_state_.current_x, anim_state_.current_y);
    const char* bound_name = boundaries[controls_.selected_boundary];
    ImGui::Text("Boundary: %s", bound_name);
    
    // Legend (dynamic based on operation)
    ImGui::SeparatorText("Legend");
    ImGui::TextColored(ImVec4(1, 1, 1, 1), "White: Foreground");
    ImGui::TextColored(ImVec4(0.3f, 0.3f, 0.3f, 1), "Dark: Background");
    ImGui::TextColored(ImVec4(1, 0.2f, 0.2f, 1), "Red: SE Center");
    ImGui::TextColored(ImVec4(1, 0.65f, 0, 1), "Orange: SE Coverage");
    
    // Result colors based on operation
    if (controls_.selected_operation == 0) {  // Erosion
        ImGui::TextColored(ImVec4(0, 0.9f, 0.4f, 1), "Green: Survives");
        ImGui::TextColored(ImVec4(0.4f, 0.2f, 0.2f, 1), "Dark Red: Eroded");
    } else if (controls_.selected_operation == 1) {  // Dilation
        ImGui::TextColored(ImVec4(0.4f, 0.6f, 1, 1), "Blue: Dilated");
        ImGui::TextColored(ImVec4(0.15f, 0.15f, 0.2f, 1), "Dark: Not expanded");
    } else {  // Boundary operations
        ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "Yellow: Edge/Boundary");
        ImGui::TextColored(ImVec4(0.15f, 0.15f, 0.15f, 1), "Dark: Not edge");
    }
    
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Visualizer::stepAnimation(AnimationState& state, 
                               const BinaryImage& original,
                               BinaryImage& result,
                               const Morphology& morph) {
    if (state.completed || state.paused) {
        return;
    }

    Uint32 current_time = SDL_GetTicks();
    if (current_time - state.last_step_time < static_cast<Uint32>(state.speed_ms)) {
        return;
    }
    state.last_step_time = current_time;

    bool survives = morph.checkPixel(original, state.current_x, state.current_y);
    result.set(state.current_x, state.current_y, survives);

    state.current_x++;
    if (state.current_x >= original.width()) {
        state.current_x = 0;
        state.current_y++;
        if (state.current_y >= original.height()) {
            state.completed = true;
        }
    }
}

void Visualizer::resetAnimation() {
    anim_state_.current_x = 0;
    anim_state_.current_y = 0;
    anim_state_.completed = false;
    anim_state_.paused = true;
    if (result_image_) {
        result_image_->clear();
    }
}

bool Visualizer::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        
        switch (event.type) {
            case SDL_QUIT:
                return false;

            case SDL_KEYDOWN:
                if (!ImGui::GetIO().WantCaptureKeyboard) {
                    switch (event.key.keysym.sym) {
                        case SDLK_ESCAPE:
                        case SDLK_q:
                            return false;
                        case SDLK_SPACE:
                            anim_state_.paused = !anim_state_.paused;
                            break;
                        case SDLK_r:
                            controls_.needs_regenerate = true;
                            break;
                        case SDLK_e:
                            controls_.selected_operation = 0;
                            controls_.needs_regenerate = true;
                            break;
                        case SDLK_d:
                            controls_.selected_operation = 1;
                            controls_.needs_regenerate = true;
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

void Visualizer::run(std::function<BinaryImage(const UIControls&)> createImageFunc) {
    current_image_ = std::make_unique<BinaryImage>(createImageFunc(controls_));
    result_image_ = std::make_unique<BinaryImage>(current_image_->width(), current_image_->height(), false);
    
    // Create morphology processor with current settings
    StructuringElement se = controls_.se_is_cross ? 
        StructuringElement::createCross(controls_.se_size) :
        StructuringElement::createSquare(controls_.se_size);
    
    MorphOperation op = static_cast<MorphOperation>(controls_.selected_operation);
    
    BoundaryMode boundary = static_cast<BoundaryMode>(controls_.selected_boundary);
    
    morphology_ = std::make_unique<Morphology>(se, op, boundary);

    std::cout << "\n=== Morphological Operations - Interactive Demo ===\n";
    std::cout << "Use the ImGui control panel to:\n";
    std::cout << "  - Switch between Erosion and Dilation\n";
    std::cout << "  - Change boundary handling mode\n";
    std::cout << "  - Adjust grid size and shape\n";
    std::cout << "  - Control animation speed\n\n";
    std::cout << "Keyboard shortcuts:\n";
    std::cout << "  SPACE - Pause/Resume\n";
    std::cout << "  E     - Switch to Erosion\n";
    std::cout << "  D     - Switch to Dilation\n";
    std::cout << "  R     - Reset/Regenerate\n";
    std::cout << "  ESC   - Quit\n\n";

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
            int control_panel_width = 280;
            window_width_ = control_panel_width + panel_width * 2 + gap_ * 4;
            window_height_ = std::max(panel_height + gap_ * 2 + 40, 600);
            
            SDL_SetWindowSize(window_, window_width_, window_height_);
            
            controls_.needs_regenerate = true;
        }
        
        if (controls_.needs_regenerate) {
            controls_.needs_regenerate = false;
            
            current_image_ = std::make_unique<BinaryImage>(createImageFunc(controls_));
            image_width_ = current_image_->width();
            image_height_ = current_image_->height();
            result_image_ = std::make_unique<BinaryImage>(image_width_, image_height_, false);
            
            StructuringElement se = controls_.se_is_cross ? 
                StructuringElement::createCross(controls_.se_size) :
                StructuringElement::createSquare(controls_.se_size);
            
            MorphOperation op = static_cast<MorphOperation>(controls_.selected_operation);
            
            BoundaryMode boundary = static_cast<BoundaryMode>(controls_.selected_boundary);
            
            morphology_ = std::make_unique<Morphology>(se, op, boundary);
            
            resetAnimation();
        }
        
        if (!anim_state_.paused && !anim_state_.completed) {
            stepAnimation(anim_state_, *current_image_, *result_image_, *morphology_);
        }

        int display_w, display_h;
        SDL_GL_GetDrawableSize(window_, &display_w, &display_h);
        
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        renderImGui(*current_image_, *result_image_);
        
        SDL_GL_SwapWindow(window_);
        
        SDL_Delay(10);
    }
}
