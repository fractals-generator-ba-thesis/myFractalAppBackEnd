#pragma once
#include <array>
#include <cstdint>
#include <cmath>
#include <algorithm>

std::array<uint8_t, 4> color(int iter, int max_iter, double x, double y) {
    if (iter == max_iter) return {0, 0, 0, 255};

    double smooth = iter + 1 - log2(log2(x*x + y*y));
    double t = smooth / max_iter;

    uint8_t r = (uint8_t)(std::clamp(9   * (1-t) * t*t*t,       0.0, 1.0) * 255);
    uint8_t g = (uint8_t)(std::clamp(15  * (1-t)*(1-t) * t*t,   0.0, 1.0) * 255);
    uint8_t b = (uint8_t)(std::clamp(8.5 * (1-t)*(1-t)*(1-t)*t, 0.0, 1.0) * 255);
    return {r, g, b, 255};
}