#ifndef FLOODFILL_HPP
#define FLOODFILL_HPP

#include "binary_image.hpp"
#include <queue>
#include <stack>
#include <vector>
#include <utility>
#include <cmath>

// Connectivity options for neighbor lookup
enum class Connectivity {
    Four,   // N, S, E, W only
    Eight   // Includes diagonals
};

// Traversal strategy
enum class FillAlgorithm {
    BFS,    // Queue-based, spreads uniformly
    DFS     // Stack-based, explores depth first
};

// Pixel states during fill animation
enum class PixelState : uint8_t {
    Unvisited,
    InQueue,
    Processed,
    Boundary,
    Unsafe      // Position too close to boundary for safety radius
};

/**
 * Flood fill with optional safety radius constraint.
 *
 * When safety_radius > 0, only positions where a disk of that radius
 * fits entirely within the fillable region are considered valid.
 * This is useful for determining navigable areas with clearance
 * from obstacles (e.g., robot path planning, VR play space detection).
 */
class FloodFill {
public:
    FloodFill(Connectivity connectivity = Connectivity::Four,
              FillAlgorithm algorithm = FillAlgorithm::BFS,
              int safety_radius = 0);

    // Reset and start fill from given position
    void initialize(const BinaryImage& image, int start_x, int start_y);

    // Process next pixel in queue/stack. Returns false when done.
    bool step();

    bool isComplete() const { return frontier_.empty() && initialized_; }

    PixelState getState(int x, int y) const;

    // Check if disk of radius R fits at position. All pixels in disk
    // must be foreground for this to return true.
    bool checkCircleFits(int center_x, int center_y) const;

    // Get disk pixel positions for visualization
    std::vector<std::pair<int, int>> getCirclePositions(int center_x, int center_y) const;

    // Accessors
    std::pair<int, int> getCurrentPixel() const { return current_pixel_; }
    size_t getFrontierSize() const { return frontier_.size(); }
    size_t getFilledCount() const { return filled_count_; }
    size_t getUnsafeCount() const { return unsafe_count_; }
    std::vector<std::pair<int, int>> getFrontierPositions() const;
    const BinaryImage& getResult() const { return result_; }
    const BinaryImage& getSafetyMask() const { return safety_mask_; }
    const std::vector<std::pair<int, int>>& getNeighborOffsets() const { return offsets_; }
    
    Connectivity getConnectivity() const { return connectivity_; }
    FillAlgorithm getAlgorithm() const { return algorithm_; }
    int getSafetyRadius() const { return safety_radius_; }
    
    void setConnectivity(Connectivity c) { connectivity_ = c; updateOffsets(); }
    void setAlgorithm(FillAlgorithm a) { algorithm_ = a; }
    void setSafetyRadius(int r) { safety_radius_ = r; updateDiskOffsets(); }

private:
    void updateOffsets();
    void updateDiskOffsets();
    void precomputeSafetyMask();
    bool isValid(int x, int y) const;

    Connectivity connectivity_;
    FillAlgorithm algorithm_;
    int safety_radius_;
    
    std::vector<std::pair<int, int>> offsets_;
    std::vector<std::pair<int, int>> disk_offsets_;
    
    BinaryImage source_;
    BinaryImage result_;
    BinaryImage safety_mask_;
    std::vector<std::vector<PixelState>> state_;
    
    std::deque<std::pair<int, int>> frontier_;
    
    std::pair<int, int> current_pixel_{-1, -1};
    bool target_value_ = false;
    bool initialized_ = false;
    size_t filled_count_ = 0;
    size_t unsafe_count_ = 0;
    int width_ = 0;
    int height_ = 0;
};

#endif
