cmake_minimum_required(VERSION 3.20)
project(mandelbrot_server)

set(CMAKE_CXX_STANDARD 20)

set(OpenMP_C_FLAGS "-fopenmp")
set(OpenMP_CXX_FLAGS "-fopenmp")
set(OpenMP_C_LIB_NAMES "omp")
set(OpenMP_CXX_LIB_NAMES "omp")
set(OpenMP_omp_LIBRARY /opt/homebrew/opt/libomp/lib/libomp.dylib)

find_package(OpenMP REQUIRED)

add_executable(server main.cpp)

target_include_directories(server PRIVATE
        include
        /opt/homebrew/opt/boost/include    # ← adds Boost headers
)

target_link_libraries(server PRIVATE OpenMP::OpenMP_CXX)

target_compile_options(server PRIVATE -O3 -march=native -ffast-math)