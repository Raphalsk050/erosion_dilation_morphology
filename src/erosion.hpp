#ifndef MORPHOLOGY_HPP
#define MORPHOLOGY_HPP

#include "binary_image.hpp"
#include <vector>
#include <utility>

/**
 * @brief Boundary handling modes for morphological operations.
 */
enum class BoundaryMode {
    Zero,      ///< Out-of-bounds pixels are treated as 0 (background)
    One,       ///< Out-of-bounds pixels are treated as 1 (foreground)
    Extend,    ///< Extend edge pixels (clamp to nearest border pixel)
    Wrap       ///< Wrap around to opposite edge (periodic boundary)
};

/**
 * @brief Type of morphological operation.
 */
enum class MorphOperation {
    Erosion,        ///< Shrinks foreground (output=1 only if ALL neighbors are 1)
    Dilation,       ///< Expands foreground (output=1 if ANY neighbor is 1)
    InnerBoundary,  ///< Internal edge: Original - Eroded (contour inside shape)
    OuterBoundary,  ///< External edge: Dilated - Original (contour outside shape)
    Gradient        ///< Morphological gradient: Dilated - Eroded (full edge)
};

/**
 * @brief Represents a structuring element for morphological operations.
 */
struct StructuringElement {
    int width;
    int height;
    int center_x;
    int center_y;
    std::vector<std::pair<int, int>> offsets;

    static StructuringElement createSquare(int size);
    static StructuringElement createCross(int size);
};

/**
 * @brief Performs morphological operations on binary images.
 * 
 * Supports:
 * - Erosion: Shrinks foreground
 * - Dilation: Expands foreground  
 * - Inner Boundary: Original - Eroded (internal contour)
 * - Outer Boundary: Dilated - Original (external contour)
 * - Gradient: Dilated - Eroded (full edge/thickness)
 */
class Morphology {
public:
    Morphology(const StructuringElement& se, 
               MorphOperation op = MorphOperation::Erosion,
               BoundaryMode boundary = BoundaryMode::Zero);

    /**
     * @brief Perform the morphological operation on entire image.
     */
    BinaryImage apply(const BinaryImage& input) const;

    /**
     * @brief Check result for a single pixel (for animated step-by-step).
     * 
     * Note: For boundary operations, this computes the result by
     * checking erosion/dilation of the pixel and comparing with original.
     */
    bool checkPixel(const BinaryImage& input, int x, int y) const;

    /**
     * @brief Get pixel value with boundary handling.
     */
    bool getPixelWithBoundary(const BinaryImage& input, int x, int y) const;

    /**
     * @brief Get positions covered by SE at given location.
     */
    std::vector<std::pair<int, int>> getCoveredPositions(int x, int y) const;

    // Getters
    const StructuringElement& getStructuringElement() const { return se_; }
    MorphOperation getOperation() const { return operation_; }
    BoundaryMode getBoundaryMode() const { return boundary_; }

    // Setters
    void setOperation(MorphOperation op) { operation_ = op; }
    void setBoundaryMode(BoundaryMode mode) { boundary_ = mode; }

private:
    // Helper functions for erosion/dilation at a single pixel
    bool checkErosion(const BinaryImage& input, int x, int y) const;
    bool checkDilation(const BinaryImage& input, int x, int y) const;

    StructuringElement se_;
    MorphOperation operation_;
    BoundaryMode boundary_;
};

using Erosion = Morphology;

#endif // MORPHOLOGY_HPP
