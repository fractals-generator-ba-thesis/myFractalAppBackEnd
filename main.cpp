#include "httplib.h"
#include "json.hpp"
#include "src/render.h"
#include "src/tile.h"
#include "src/julia.h"
#include "src/barnsley.h"


int main() {
    httplib::Server svr;

    auto set_cors = [](httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
    };

    // ── /render ──────────────────────────────────────────────────────────────
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
        res.set_content(reinterpret_cast<char*>(pixels.data()),
                        pixels.size(), "application/octet-stream");
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
        res.set_content(reinterpret_cast<char*>(pixels.data()),
                        pixels.size(), "application/octet-stream");
    });

    // ── /render-julia ─────────────────────────────────────────────────────────
    svr.Options("/render-julia", [&](const httplib::Request&, httplib::Response& res) {
        set_cors(res); res.status = 204;
    });

    svr.Post("/render-julia", [&](const httplib::Request& req, httplib::Response& res) {
        auto body = nlohmann::json::parse(req.body);
        JuliaRequest j;
        j.tile_size = body["tile_size"];
        j.max_iter  = body["max_iter"];
        j.c_u       = body["c_u"];
        j.c_v       = body["c_v"];
        j.u_min     = body.value("u_min", -2.0);
        j.u_max     = body.value("u_max",  2.0);
        j.v_min     = body.value("v_min", -2.0);
        j.v_max     = body.value("v_max",  2.0);

        auto pixels = render_julia(j);
        set_cors(res);
        res.set_content(reinterpret_cast<char*>(pixels.data()),
                        pixels.size(), "application/octet-stream");
    });

    svr.Options("/render-barnsley", [&](const httplib::Request&, httplib::Response& res) {
    set_cors(res); res.status = 204;
});

    svr.Post("/render-barnsley", [&](const httplib::Request& req, httplib::Response& res) {
        auto body = nlohmann::json::parse(req.body);

        BarnsleyRequest b;
        b.width      = body["width"];
        b.height     = body["height"];
        b.iterations = body.value("iterations", 100000);

        auto pixels = render_barnsley(b);
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