#include "httplib.h"
#include "json.hpp"
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <vector>
#include <cmath>
#include <algorithm>

using hp = boost::multiprecision::cpp_dec_float_50;

// ─── Structures ───────────────────────────────────────────────────────────────

struct RenderRequest {
    int width, height, max_iter;
    double offset_u, offset_v, zoom;
};

struct TileRequest {
    int x, y, z, tile_size, max_iter;
};

// ─── Color ────────────────────────────────────────────────────────────────────

std::array<uint8_t, 4> color(int iter, int max_iter, double x, double y) {
    if (iter == max_iter) return {0, 0, 0, 255};

    double smooth = iter + 1 - log2(log2(x*x + y*y));
    double t = smooth / max_iter;

    uint8_t r = (uint8_t)(std::clamp(9   * (1-t) * t*t*t,       0.0, 1.0) * 255);
    uint8_t g = (uint8_t)(std::clamp(15  * (1-t)*(1-t) * t*t,   0.0, 1.0) * 255);
    uint8_t b = (uint8_t)(std::clamp(8.5 * (1-t)*(1-t)*(1-t)*t, 0.0, 1.0) * 255);
    return {r, g, b, 255};
}

// ─── Mandelbrot (double) ──────────────────────────────────────────────────────

int mandelbrot(double u, double v, int max_iter, double& out_x, double& out_y) {
    double x = 0.0, y = 0.0;
    for (int i = 0; i < max_iter; i++) {
        if (x*x + y*y > 4.0) {
            out_x = x; out_y = y;
            return i;
        }
        double xn = x*x - y*y + u;
        y = 2.0*x*y + v;
        x = xn;
    }
    out_x = x; out_y = y;
    return max_iter;
}

// ─── Mandelbrot (high precision) ─────────────────────────────────────────────

int mandelbrot_hp(hp u, hp v, int max_iter, double& out_x, double& out_y) {
    hp x = 0, y = 0;
    for (int i = 0; i < max_iter; i++) {
        if (x*x + y*y > 4) {
            out_x = (double)x;
            out_y = (double)y;
            return i;
        }
        hp xn = x*x - y*y + u;
        y = 2*x*y + v;
        x = xn;
    }
    out_x = (double)x;
    out_y = (double)y;
    return max_iter;
}

// ─── Full frame render ────────────────────────────────────────────────────────

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

// ─── Tile helpers ─────────────────────────────────────────────────────────────

void tile_to_bounds(const TileRequest& t,
    double& u_min, double& u_max,
    double& v_min, double& v_max)
{
    int n = 1 << t.z;  // 2^z
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

// ─── Tile render ─────────────────────────────────────────────────────────────

std::vector<uint8_t> render_tile(const TileRequest& req) {
    double u_min, u_max, v_min, v_max;
    tile_to_bounds(req, u_min, u_max, v_min, v_max);

    std::vector<uint8_t> pixels(req.tile_size * req.tile_size * 4);

    // Rectangle check — fill solid black if entire perimeter is inside set
    if (perimeter_all_inside(u_min, u_max, v_min, v_max,
                              req.tile_size, req.max_iter)) {
        for (int i = 0; i < req.tile_size * req.tile_size; i++) {
            pixels[i*4]   = 0;
            pixels[i*4+1] = 0;
            pixels[i*4+2] = 0;
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

// ─── Main ─────────────────────────────────────────────────────────────────────

int main() {
    httplib::Server svr;

    // CORS helper
    auto set_cors = [](httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
    };

    // ── /render (full frame) ──────────────────────────────────────────────────
    svr.Options("/render", [&](const httplib::Request&, httplib::Response& res) {
        set_cors(res); res.status = 204;
    });

    svr.Post("/render", [&](const httplib::Request& req, httplib::Response& res) {
        auto body = nlohmann::json::parse(req.body);

        RenderRequest r;
        r.width    = body["width"];
        r.height   = body["height"];
        r.offset_u = body["offset_u"];
        r.offset_v = body["offset_v"];
        r.zoom     = body["zoom"];
        r.max_iter = body["max_iter"];

        auto pixels = render(r);
        set_cors(res);
        res.set_content(
            reinterpret_cast<char*>(pixels.data()),
            pixels.size(),
            "application/octet-stream"
        );
    });

    // ── /render-tile ──────────────────────────────────────────────────────────
    svr.Options("/render-tile", [&](const httplib::Request&, httplib::Response& res) {
        set_cors(res); res.status = 204;
    });

    svr.Post("/render-tile", [&](const httplib::Request& req, httplib::Response& res) {
        auto body = nlohmann::json::parse(req.body);

        TileRequest t;
        t.x         = body["x"];
        t.y         = body["y"];
        t.z         = body["z"];
        t.tile_size = body["tile_size"];
        t.max_iter  = body["max_iter"];

        auto pixels = render_tile(t);
        set_cors(res);
        res.set_content(
            reinterpret_cast<char*>(pixels.data()),
            pixels.size(),
            "application/octet-stream"
        );
    });

    printf("Server running on http://localhost:3001\n");
    svr.listen("0.0.0.0", 3001);
}