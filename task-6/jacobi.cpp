#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <chrono>

#include <boost/program_options.hpp>

#ifdef _OPENACC
#include <openacc.h>
#endif

namespace po = boost::program_options;

static inline int idx(int i, int j, int N)
{
    return i * N + j;
}

void initialize_grid(std::vector<double>& grid, int N)
{
    // углы:
    // 10 --- 20
    // |       |
    // 20 --- 30

    double tl = 10.0;
    double tr = 20.0;
    double br = 30.0;
    double bl = 20.0;

    // верхняя граница
    for (int j = 0; j < N; j++)
    {
        grid[idx(0, j, N)] =
            tl + (tr - tl) * j / (double)(N - 1);
    }

    // нижняя граница
    for (int j = 0; j < N; j++)
    {
        grid[idx(N - 1, j, N)] =
            bl + (br - bl) * j / (double)(N - 1);
    }

    // левая граница
    for (int i = 0; i < N; i++)
    {
        grid[idx(i, 0, N)] =
            tl + (bl - tl) * i / (double)(N - 1);
    }

    // правая граница
    for (int i = 0; i < N; i++)
    {
        grid[idx(i, N - 1, N)] =
            tr + (br - tr) * i / (double)(N - 1);
    }
}

void print_grid(const std::vector<double>& grid, int N)
{
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            std::cout
                << std::setw(12)
                << std::fixed
                << std::setprecision(6)
                << grid[idx(i, j, N)];
        }

        std::cout << "\n";
    }
}

int main(int argc, char* argv[])
{
    int N;
    double eps;
    int max_iters;

    po::options_description desc("Options");

    desc.add_options()
        ("help", "help")
        ("size", po::value<int>(&N)->default_value(128))
        ("eps", po::value<double>(&eps)->default_value(1e-6))
        ("max-iters", po::value<int>(&max_iters)->default_value(1000000));

    po::variables_map vm;

    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << desc << "\n";
        return 0;
    }

    std::vector<double> grid(N * N, 0.0);
    std::vector<double> new_grid(N * N, 0.0);

    initialize_grid(grid, N);
    initialize_grid(new_grid, N);

    int iterations = 0;
    double error = 0.0;

    auto start = std::chrono::high_resolution_clock::now();

#pragma acc data copy(grid[0:N*N], new_grid[0:N*N])
    {
        for (iterations = 0; iterations < max_iters; iterations++)
        {
            error = 0.0;

#pragma acc parallel loop collapse(2) reduction(max:error)
            for (int i = 1; i < N - 1; i++)
            {
                for (int j = 1; j < N - 1; j++)
                {
                    int id = idx(i, j, N);

                    new_grid[id] = 0.25 * (
                        grid[idx(i - 1, j, N)] +
                        grid[idx(i + 1, j, N)] +
                        grid[idx(i, j - 1, N)] +
                        grid[idx(i, j + 1, N)]
                    );

                    double diff =
                        fabs(new_grid[id] - grid[id]);

                    if (diff > error)
                    {
                        error = diff;
                    }
                }
            }

#pragma acc parallel loop
            for (int k = 0; k < N * N; k++)
            {
                grid[k] = new_grid[k];
            }

            if (error < eps)
            {
                break;
            }
        }
    }

    auto end = std::chrono::high_resolution_clock::now();

    double elapsed =
        std::chrono::duration<double>(end - start).count();

    std::cout << "Grid size: "
              << N << "x" << N << "\n";

    std::cout << "Iterations: "
              << iterations << "\n";

    std::cout << "Error: "
              << std::scientific
              << error << "\n";

    std::cout << "Time: "
              << std::fixed
              << elapsed
              << " sec\n";

    if (N == 10 || N == 13)
    {
        std::cout << "Grid:\n";
        print_grid(grid, N);
    }

    return 0;
}