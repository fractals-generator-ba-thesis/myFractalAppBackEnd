#pragma once
#include <vector>
#include <cstdint>
#include "mandelbrot.h"
#include "color.h"

struct TileRequest {
    int x, y, z, tile_size, max_iter;
};

void tile_to_bounds(const TileRequest& t,
    double& u_min, double& u_max,
    double& v_min, double& v_max)
{
    int n = 1 << t.z;
    double tile_w = 3.5 / n;
    double tile_h = 3.5 / n;

    u_min = -2.5 + t.x * tile_w;
    u_max = u_min + tile_w;
    v_min = -1.25 + t.y * tile_h;
    v_max = v_min + tile_h;
}

bool perimeter_all_inside(double u_min, double u_max,
                           double v_min, double v_max,
                           int size, int max_iter)
{
    auto inside = [&](double u, double v) {
        double x = 0, y = 0;
        for (int i = 0; i < max_iter; i++) {
            if (x*x + y*y > 4.0) return false;
            double xn = x*x - y*y + u;
            y = 2*x*y + v; x = xn;
        }
        return true;
    };

    for (int i = 0; i <= size; i++) {
        double t = (double)i / size;
        double u = u_min + t * (u_max - u_min);
        double v = v_min + t * (v_max - v_min);
        if (!inside(u, v_min)) return false;
        if (!inside(u, v_max)) return false;
        if (!inside(u_min, v)) return false;
        if (!inside(u_max, v)) return false;
    }
    return true;
}

std::vector<uint8_t> render_tile(const TileRequest& req) {
    double u_min, u_max, v_min, v_max;
    tile_to_bounds(req, u_min, u_max, v_min, v_max);

    std::vector<uint8_t> pixels(req.tile_size * req.tile_size * 4);

    if (perimeter_all_inside(u_min, u_max, v_min, v_max,
                              req.tile_size, req.max_iter)) {
        for (int i = 0; i < req.tile_size * req.tile_size; i++) {
            pixels[i*4] = pixels[i*4+1] = pixels[i*4+2] = 0;
            pixels[i*4+3] = 255;
        }
        return pixels;
    }

    #pragma omp parallel for schedule(dynamic)
    for (int py = 0; py < req.tile_size; py++) {
        for (int px = 0; px < req.tile_size; px++) {
            double u = u_min + (px / (double)req.tile_size) * (u_max - u_min);
            double v = v_min + (py / (double)req.tile_size) * (v_max - v_min);

            double ox, oy;
            int iter = mandelbrot(u, v, req.max_iter, ox, oy);
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