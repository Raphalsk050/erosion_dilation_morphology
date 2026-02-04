#include "binary_image.hpp"
#include <cmath>
#include <algorithm>

BinaryImage::BinaryImage(int width, int height, bool fill_value)
    : width_(width)
    , height_(height)
    , pixels_(width * height, fill_value)
{
}

bool BinaryImage::get(int x, int y) const {
    // Return false (background) for out-of-bounds access
    // This is important for erosion at image borders
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        return false;
    }
    return pixels_[y * width_ + x];
}

void BinaryImage::set(int x, int y, bool value) {
    if (x >= 0 && x < width_ && y >= 0 && y < height_) {
        pixels_[y * width_ + x] = value;
    }
}

void BinaryImage::clear() {
    std::fill(pixels_.begin(), pixels_.end(), false);
}

void BinaryImage::fill(bool value) {
    std::fill(pixels_.begin(), pixels_.end(), value);
}

BinaryImage BinaryImage::clone() const {
    BinaryImage copy(width_, height_);
    copy.pixels_ = pixels_;
    return copy;
}

BinaryImage BinaryImage::createRectangle(int width, int height, int margin) {
    BinaryImage img(width, height, false);
    
    for (int y = margin; y < height - margin; ++y) {
        for (int x = margin; x < width - margin; ++x) {
            img.set(x, y, true);
        }
    }
    
    return img;
}

BinaryImage BinaryImage::createCross(int width, int height, int thickness) {
    BinaryImage img(width, height, false);
    
    int cx = width / 2;
    int cy = height / 2;
    int half_thick = thickness / 2;
    
    // Horizontal bar
    for (int y = cy - half_thick; y <= cy + half_thick; ++y) {
        for (int x = 2; x < width - 2; ++x) {
            img.set(x, y, true);
        }
    }
    
    // Vertical bar
    for (int y = 2; y < height - 2; ++y) {
        for (int x = cx - half_thick; x <= cx + half_thick; ++x) {
            img.set(x, y, true);
        }
    }
    
    return img;
}

BinaryImage BinaryImage::createLShape(int width, int height) {
    BinaryImage img(width, height, false);
    
    int thickness = std::max(2, std::min(width, height) / 4);
    int margin = 2;
    
    // Vertical part of L
    for (int y = margin; y < height - margin; ++y) {
        for (int x = margin; x < margin + thickness; ++x) {
            img.set(x, y, true);
        }
    }
    
    // Horizontal part of L (bottom)
    for (int y = height - margin - thickness; y < height - margin; ++y) {
        for (int x = margin; x < width - margin; ++x) {
            img.set(x, y, true);
        }
    }
    
    return img;
}

BinaryImage BinaryImage::createCircle(int width, int height, int radius) {
    BinaryImage img(width, height, false);
    
    int cx = width / 2;
    int cy = height / 2;
    int r2 = radius * radius;
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int dx = x - cx;
            int dy = y - cy;
            if (dx * dx + dy * dy <= r2) {
                img.set(x, y, true);
            }
        }
    }
    
    return img;
}

// Simple Perlin-like noise implementation
namespace {
    // Fade function for smooth interpolation
    float fade(float t) {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    // Linear interpolation
    float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }

    // Pseudo-random gradient
    float grad(int hash, float x, float y) {
        int h = hash & 15;
        float u = h < 8 ? x : y;
        float v = h < 4 ? y : (h == 12 || h == 14 ? x : 0);
        return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
    }

    // Permutation table
    class PerlinNoise {
    public:
        PerlinNoise(unsigned int seed) {
            // Initialize permutation table
            for (int i = 0; i < 256; ++i) {
                p[i] = i;
            }
            // Shuffle using seed
            std::srand(seed);
            for (int i = 255; i > 0; --i) {
                int j = std::rand() % (i + 1);
                std::swap(p[i], p[j]);
            }
            // Duplicate for overflow
            for (int i = 0; i < 256; ++i) {
                p[256 + i] = p[i];
            }
        }

        float noise(float x, float y) const {
            // Find unit grid cell
            int X = static_cast<int>(std::floor(x)) & 255;
            int Y = static_cast<int>(std::floor(y)) & 255;

            // Relative position in cell
            x -= std::floor(x);
            y -= std::floor(y);

            // Fade curves
            float u = fade(x);
            float v = fade(y);

            // Hash coordinates
            int aa = p[p[X] + Y];
            int ab = p[p[X] + Y + 1];
            int ba = p[p[X + 1] + Y];
            int bb = p[p[X + 1] + Y + 1];

            // Interpolate
            float res = lerp(
                lerp(grad(aa, x, y), grad(ba, x - 1, y), u),
                lerp(grad(ab, x, y - 1), grad(bb, x - 1, y - 1), u),
                v
            );

            // Normalize to 0-1 range
            return (res + 1.0f) / 2.0f;
        }

        // Fractal Brownian Motion for more interesting patterns
        float fbm(float x, float y, int octaves = 4) const {
            float value = 0.0f;
            float amplitude = 0.5f;
            float frequency = 1.0f;
            
            for (int i = 0; i < octaves; ++i) {
                value += amplitude * noise(x * frequency, y * frequency);
                amplitude *= 0.5f;
                frequency *= 2.0f;
            }
            
            return value;
        }

    private:
        int p[512];
    };
}

BinaryImage BinaryImage::createNoise(int width, int height, float scale, 
                                      float threshold, unsigned int seed) {
    BinaryImage img(width, height, false);
    PerlinNoise perlin(seed);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Use FBM for more complex patterns
            float noise_val = perlin.fbm(x * scale, y * scale, 3);
            
            // Apply threshold
            if (noise_val > threshold) {
                img.set(x, y, true);
            }
        }
    }

    return img;
}

