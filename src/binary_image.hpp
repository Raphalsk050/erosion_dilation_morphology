#ifndef BINARY_IMAGE_HPP
#define BINARY_IMAGE_HPP

#include <vector>
#include <cstdint>
#include <string>

/**
 * @brief Represents a binary image (black and white only).
 * 
 * Each pixel is either 0 (background/black) or 1 (foreground/white).
 * This is the fundamental data structure for morphological operations.
 */
class BinaryImage {
public:
    /**
     * @brief Construct a new Binary Image with given dimensions.
     * @param width Image width in pixels
     * @param height Image height in pixels
     * @param fill_value Initial value for all pixels (default: false/0)
     */
    BinaryImage(int width, int height, bool fill_value = false);

    /**
     * @brief Get pixel value at specified position.
     * @param x Column index (0 to width-1)
     * @param y Row index (0 to height-1)
     * @return true if pixel is foreground (1), false if background (0)
     */
    bool get(int x, int y) const;

    /**
     * @brief Set pixel value at specified position.
     * @param x Column index
     * @param y Row index
     * @param value Value to set (true=foreground, false=background)
     */
    void set(int x, int y, bool value);

    /**
     * @brief Get image width.
     * @return Width in pixels
     */
    int width() const { return width_; }

    /**
     * @brief Get image height.
     * @return Height in pixels
     */
    int height() const { return height_; }

    /**
     * @brief Clear the image (set all pixels to background).
     */
    void clear();

    /**
     * @brief Fill entire image with a value.
     * @param value Value to fill with
     */
    void fill(bool value);

    /**
     * @brief Create a copy of this image.
     * @return New BinaryImage with same content
     */
    BinaryImage clone() const;

    // Factory methods to create sample images for demonstration

    /**
     * @brief Create a filled rectangle.
     * @param width Image width
     * @param height Image height
     * @param margin Margin from edges
     * @return BinaryImage with rectangle
     */
    static BinaryImage createRectangle(int width, int height, int margin = 2);

    /**
     * @brief Create a cross/plus shape.
     * @param width Image width
     * @param height Image height
     * @param thickness Thickness of cross arms
     * @return BinaryImage with cross
     */
    static BinaryImage createCross(int width, int height, int thickness = 3);

    /**
     * @brief Create an 'L' shape.
     * @param width Image width
     * @param height Image height
     * @return BinaryImage with L shape
     */
    static BinaryImage createLShape(int width, int height);

    /**
     * @brief Create a simple circle (approximated).
     * @param width Image width
     * @param height Image height
     * @param radius Circle radius
     * @return BinaryImage with circle
     */
    static BinaryImage createCircle(int width, int height, int radius);

    /**
     * @brief Create a complex shape using Perlin-like noise.
     * @param width Image width
     * @param height Image height
     * @param scale Noise scale (higher = more detail)
     * @param threshold Threshold for binarization (0.0 to 1.0)
     * @param seed Random seed for reproducibility
     * @return BinaryImage with organic noise pattern
     */
    static BinaryImage createNoise(int width, int height, float scale = 0.15f, 
                                   float threshold = 0.5f, unsigned int seed = 42);

private:
    int width_;
    int height_;
    std::vector<bool> pixels_;  // Row-major storage
};

#endif // BINARY_IMAGE_HPP
