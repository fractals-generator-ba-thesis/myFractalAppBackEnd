#pragma once
#include <boost/multiprecision/cpp_dec_float.hpp>

using hp = boost::multiprecision::cpp_dec_float_50;

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