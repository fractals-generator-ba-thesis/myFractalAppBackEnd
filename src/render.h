#pragma once
#include <vector>
#include <cstdint>
#include "mandelbrot.h"
#include "color.h"

struct RenderRequest {
    int width, height, max_iter;
    double offset_u, offset_v, zoom;
};

std::vector<uint8_t> render(const RenderRequest& req) {
    std::vector<uint8_t> pixels(req.width * req.height * 4);
    double scale = 3.5 / (req.width * req.zoom);

#pragma omp parallel for schedule(dynamic)
    for (int py = 0; py < req.height; py++) {
        for (int px = 0; px < req.width; px++) {
            double u = req.offset_u + (px - req.width  / 2.0) * scale;
            double v = req.offset_v + (py - req.height / 2.0) * scale;

            double ox, oy;
            int iter;

            if (req.zoom > 1e10) {
                hp u_hp = hp(req.offset_u) + hp(px - req.width  / 2.0) * hp(scale);
                hp v_hp = hp(req.offset_v) + hp(py - req.height / 2.0) * hp(scale);
                iter = mandelbrot_hp(u_hp, v_hp, req.max_iter, ox, oy);
            } else {
                iter = mandelbrot(u, v, req.max_iter, ox, oy);
            }

            auto [r, g, b, a] = color(iter, req.max_iter, ox, oy);
            int idx = (py * req.width + px) * 4;
            pixels[idx]   = r;
            pixels[idx+1] = g;
            pixels[idx+2] = b;
            pixels[idx+3] = a;
        }
    }
    return pixels;
}