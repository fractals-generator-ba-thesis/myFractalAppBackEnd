#pragma once
#include <vector>
#include <cstdint>
#include <random>
#include <cmath>

struct BarnsleyRequest {
    int width, height;
    int iterations;
};

std::vector<uint8_t> render_barnsley(const BarnsleyRequest& req) {
    std::vector<uint8_t> pixels(req.width * req.height * 4, 0);

    // Postavi sve piksele na crnu pozadinu
    for (int i = 0; i < req.width * req.height; i++) {
        pixels[i*4]   = 0;    // R
        pixels[i*4+1] = 0;    // G
        pixels[i*4+2] = 0;    // B
        pixels[i*4+3] = 255;  // A
    }

    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(1, 100);

    double x = 0.0, y = 0.0;

    for (int k = 0; k < req.iterations; k++) {
        int r = dist(rng);
        double nx, ny;

        if (r == 1) {
            // F1 — stabljika, 1%
            nx = 0.0;
            ny = 0.16 * y;
        } else if (r <= 8) {
            // F2 — donji list lijevo, 7%
            nx =  0.2  * x - 0.26 * y;
            ny =  0.23 * x + 0.22 * y + 1.6;
        } else if (r <= 15) {
            // F3 — donji list desno, 7%
            nx = -0.15 * x + 0.28 * y;
            ny =  0.26 * x + 0.24 * y + 0.44;
        } else {
            // F4 — glavni list, 85%
            nx =  0.85 * x + 0.04 * y;
            ny = -0.04 * x + 0.85 * y + 1.6;
        }

        x = nx;
        y = ny;

        // Preskoci prvih 20 iteracija
        if (k < 20) continue;

        // Mapiranje kompleksnih koordinata na piksele
        // x je u opsegu [-5, 5], y je u opsegu [0, 10]
        int px = (int)((x + 5.0) / 10.0 * req.width);
        int py = (int)((10.0 - y) / 10.0 * req.height); // flip y

        if (px >= 0 && px < req.width && py >= 0 && py < req.height) {
            int idx = (py * req.width + px) * 4;
            pixels[idx]   = 34;   // R — tamno zelena
            pixels[idx+1] = 139;  // G
            pixels[idx+2] = 34;   // B
            pixels[idx+3] = 255;  // A
        }
    }

    return pixels;
}