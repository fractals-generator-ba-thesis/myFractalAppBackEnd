#pragma once
#include <vector>
#include <cstdint>
#include "color.h"

struct JuliaRequest {
    int tile_size, max_iter;
    double c_u, c_v;
    double u_min, u_max;
    double v_min, v_max;
};

int julia(double zx, double zy, double cu, double cv,
          int max_iter, double& out_x, double& out_y)
{
    double x = zx, y = zy;
    for (int i = 0; i < max_iter; i++) {
        if (x*x + y*y > 4.0) {
            out_x = x; out_y = y;
            return i;
        }
        double xn = x*x - y*y + cu;
        y = 2.0*x*y + cv;
        x = xn;
    }
    out_x = x; out_y = y;
    return max_iter;
}

std::vector<uint8_t> render_julia(const JuliaRequest& req) {
    std::vector<uint8_t> pixels(req.tile_size * req.tile_size * 4);

#pragma omp parallel for schedule(dynamic)
    for (int py = 0; py < req.tile_size; py++) {
        for (int px = 0; px < req.tile_size; px++) {
            double zx = req.u_min + (px / (double)req.tile_size)
                        * (req.u_max - req.u_min);
            double zy = req.v_min + (py / (double)req.tile_size)
                        * (req.v_max - req.v_min);

            double ox, oy;
            int iter = julia(zx, zy, req.c_u, req.c_v, req.max_iter, ox, oy);
            auto [r, g, b, a] = color(iter, req.max_iter, ox, oy);

            int idx = (py * req.tile_size + px) * 4;
            pixels[idx]   = r;
            pixels[idx+1] = g;
            pixels[idx+2] = b;
            pixels[idx+3] = a;
        }
    }
    return pixels;
}