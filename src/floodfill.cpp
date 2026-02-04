#include "floodfill.hpp"
#include <algorithm>
#include <cmath>

FloodFill::FloodFill(Connectivity connectivity, FillAlgorithm algorithm, int safety_radius)
    : connectivity_(connectivity)
    , algorithm_(algorithm)
    , safety_radius_(safety_radius)
    , source_(1, 1, false)
    , result_(1, 1, false)
    , safety_mask_(1, 1, false)
{
    updateOffsets();
    updateDiskOffsets();
}

void FloodFill::updateOffsets() {
    offsets_.clear();
    
    // 4-connected: N, S, E, W
    offsets_ = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
    
    // 8-connected adds diagonals
    if (connectivity_ == Connectivity::Eight) {
        offsets_.push_back({-1, -1});
        offsets_.push_back({1, -1});
        offsets_.push_back({-1, 1});
        offsets_.push_back({1, 1});
    }
}

void FloodFill::updateDiskOffsets() {
    disk_offsets_.clear();
    
    if (safety_radius_ <= 0) {
        return;
    }
    
    // Generate all positions within a circle of radius R
    // Using the equation: x^2 + y^2 <= R^2
    int r = safety_radius_;
    int r_squared = r * r;
    
    for (int dy = -r; dy <= r; ++dy) {
        for (int dx = -r; dx <= r; ++dx) {
            if (dx * dx + dy * dy <= r_squared) {
                disk_offsets_.emplace_back(dx, dy);
            }
        }
    }
}

bool FloodFill::isValid(int x, int y) const {
    return x >= 0 && x < width_ && y >= 0 && y < height_;
}

bool FloodFill::checkCircleFits(int center_x, int center_y) const {
    if (safety_radius_ <= 0) {
        // No safety check - just check the pixel itself
        return isValid(center_x, center_y) && source_.get(center_x, center_y) == target_value_;
    }
    
    // Check all positions in the disk
    for (const auto& [dx, dy] : disk_offsets_) {
        int nx = center_x + dx;
        int ny = center_y + dy;
        
        // Out of bounds = not safe
        if (!isValid(nx, ny)) {
            return false;
        }
        
        // If any pixel in the disk is NOT the target value, circle doesn't fit
        if (source_.get(nx, ny) != target_value_) {
            return false;
        }
    }
    
    return true;
}

std::vector<std::pair<int, int>> FloodFill::getCirclePositions(int center_x, int center_y) const {
    std::vector<std::pair<int, int>> positions;
    
    for (const auto& [dx, dy] : disk_offsets_) {
        positions.emplace_back(center_x + dx, center_y + dy);
    }
    
    return positions;
}

void FloodFill::precomputeSafetyMask() {
    safety_mask_ = BinaryImage(width_, height_, false);
    
    if (safety_radius_ <= 0) {
        // No safety radius - all target_value pixels are safe
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                if (source_.get(x, y) == target_value_) {
                    safety_mask_.set(x, y, true);
                }
            }
        }
    } else {
        // Check each pixel to see if the circle fits
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                // Only check pixels that are the target value
                if (source_.get(x, y) == target_value_) {
                    if (checkCircleFits(x, y)) {
                        safety_mask_.set(x, y, true);
                    }
                }
            }
        }
    }
}

void FloodFill::initialize(const BinaryImage& image, int start_x, int start_y) {
    width_ = image.width();
    height_ = image.height();
    source_ = image;
    result_ = BinaryImage(width_, height_, false);
    
    // Initialize state grid
    state_.clear();
    state_.resize(height_, std::vector<PixelState>(width_, PixelState::Unvisited));
    
    // Clear frontier
    frontier_.clear();
    filled_count_ = 0;
    unsafe_count_ = 0;
    current_pixel_ = {-1, -1};
    
    // Check if start position is valid
    if (!isValid(start_x, start_y)) {
        initialized_ = false;
        return;
    }
    
    // Get the target value
    target_value_ = source_.get(start_x, start_y);
    
    // Precompute safety mask (which pixels pass the circle check)
    precomputeSafetyMask();
    
    // Check if starting position is safe
    if (!safety_mask_.get(start_x, start_y)) {
        // Starting position is not safe - mark but don't add to frontier
        state_[start_y][start_x] = PixelState::Unsafe;
        unsafe_count_++;
        initialized_ = true;
        return;
    }
    
    // Add starting pixel to frontier
    frontier_.push_back({start_x, start_y});
    state_[start_y][start_x] = PixelState::InQueue;
    
    initialized_ = true;
}

bool FloodFill::step() {
    if (!initialized_ || frontier_.empty()) {
        return false;
    }
    
    // Get next pixel based on algorithm
    std::pair<int, int> pixel;
    if (algorithm_ == FillAlgorithm::BFS) {
        pixel = frontier_.front();
        frontier_.pop_front();
    } else {
        pixel = frontier_.back();
        frontier_.pop_back();
    }
    
    int x = pixel.first;
    int y = pixel.second;
    current_pixel_ = pixel;
    
    // Mark as processed and fill
    state_[y][x] = PixelState::Processed;
    result_.set(x, y, true);
    filled_count_++;
    
    // Check all neighbors
    for (const auto& [dx, dy] : offsets_) {
        int nx = x + dx;
        int ny = y + dy;
        
        if (!isValid(nx, ny)) {
            continue;
        }
        
        // Skip if already visited
        if (state_[ny][nx] != PixelState::Unvisited) {
            continue;
        }
        
        // Check if neighbor has the same value
        if (source_.get(nx, ny) != target_value_) {
            // This is a boundary pixel
            state_[ny][nx] = PixelState::Boundary;
            continue;
        }
        
        // Check if the safety circle fits at this position
        if (!safety_mask_.get(nx, ny)) {
            // Circle doesn't fit - mark as unsafe but don't add to frontier
            state_[ny][nx] = PixelState::Unsafe;
            unsafe_count_++;
            continue;
        }
        
        // Safe to fill - add to frontier
        frontier_.push_back({nx, ny});
        state_[ny][nx] = PixelState::InQueue;
    }
    
    return !frontier_.empty();
}

PixelState FloodFill::getState(int x, int y) const {
    if (!isValid(x, y)) {
        return PixelState::Unvisited;
    }
    return state_[y][x];
}

std::vector<std::pair<int, int>> FloodFill::getFrontierPositions() const {
    return std::vector<std::pair<int, int>>(frontier_.begin(), frontier_.end());
}
