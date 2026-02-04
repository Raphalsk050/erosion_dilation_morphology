#include "erosion.hpp"
#include <algorithm>

StructuringElement StructuringElement::createSquare(int size) {
    StructuringElement se;
    se.width = size;
    se.height = size;
    se.center_x = size / 2;
    se.center_y = size / 2;

    for (int dy = -se.center_y; dy <= se.center_y; ++dy) {
        for (int dx = -se.center_x; dx <= se.center_x; ++dx) {
            se.offsets.emplace_back(dx, dy);
        }
    }

    return se;
}

StructuringElement StructuringElement::createCross(int size) {
    StructuringElement se;
    se.width = size;
    se.height = size;
    se.center_x = size / 2;
    se.center_y = size / 2;

    // Center pixel
    se.offsets.emplace_back(0, 0);

    // Horizontal arm
    for (int dx = -se.center_x; dx <= se.center_x; ++dx) {
        if (dx != 0) {
            se.offsets.emplace_back(dx, 0);
        }
    }

    // Vertical arm
    for (int dy = -se.center_y; dy <= se.center_y; ++dy) {
        if (dy != 0) {
            se.offsets.emplace_back(0, dy);
        }
    }

    return se;
}

Morphology::Morphology(const StructuringElement& se, MorphOperation op, BoundaryMode boundary)
    : se_(se)
    , operation_(op)
    , boundary_(boundary)
{
}

bool Morphology::getPixelWithBoundary(const BinaryImage& input, int x, int y) const {
    int w = input.width();
    int h = input.height();

    if (x >= 0 && x < w && y >= 0 && y < h) {
        return input.get(x, y);
    }

    switch (boundary_) {
        case BoundaryMode::Zero:
            return false;

        case BoundaryMode::One:
            return true;

        case BoundaryMode::Extend:
            x = std::clamp(x, 0, w - 1);
            y = std::clamp(y, 0, h - 1);
            return input.get(x, y);

        case BoundaryMode::Wrap:
            x = ((x % w) + w) % w;
            y = ((y % h) + h) % h;
            return input.get(x, y);

        default:
            return false;
    }
}

bool Morphology::checkErosion(const BinaryImage& input, int x, int y) const {
    // Erosion: output=1 only if ALL pixels under SE are 1
    for (const auto& [dx, dy] : se_.offsets) {
        if (!getPixelWithBoundary(input, x + dx, y + dy)) {
            return false;
        }
    }
    return true;
}

bool Morphology::checkDilation(const BinaryImage& input, int x, int y) const {
    // Dilation: output=1 if ANY pixel under SE is 1
    for (const auto& [dx, dy] : se_.offsets) {
        if (getPixelWithBoundary(input, x + dx, y + dy)) {
            return true;
        }
    }
    return false;
}

bool Morphology::checkPixel(const BinaryImage& input, int x, int y) const {
    bool original = input.get(x, y);
    
    switch (operation_) {
        case MorphOperation::Erosion:
            return checkErosion(input, x, y);

        case MorphOperation::Dilation:
            return checkDilation(input, x, y);

        case MorphOperation::InnerBoundary:
            // Inner boundary: Original AND NOT Eroded
            // Pixels that are in original but would be eroded
            return original && !checkErosion(input, x, y);

        case MorphOperation::OuterBoundary:
            // Outer boundary: Dilated AND NOT Original
            // Pixels that are added by dilation
            return checkDilation(input, x, y) && !original;

        case MorphOperation::Gradient:
            // Morphological gradient: Dilated XOR Eroded
            // Full edge (both inner and outer)
            return checkDilation(input, x, y) != checkErosion(input, x, y);

        default:
            return original;
    }
}

BinaryImage Morphology::apply(const BinaryImage& input) const {
    BinaryImage output(input.width(), input.height(), false);

    for (int y = 0; y < input.height(); ++y) {
        for (int x = 0; x < input.width(); ++x) {
            output.set(x, y, checkPixel(input, x, y));
        }
    }

    return output;
}

std::vector<std::pair<int, int>> Morphology::getCoveredPositions(int x, int y) const {
    std::vector<std::pair<int, int>> positions;
    positions.reserve(se_.offsets.size());

    for (const auto& [dx, dy] : se_.offsets) {
        positions.emplace_back(x + dx, y + dy);
    }

    return positions;
}
