#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(USE_BOOST) && __has_include(<boost/program_options.hpp>)
#include <boost/program_options.hpp>
#define TASK6_HAS_BOOST 1
#else
#define TASK6_HAS_BOOST 0
#endif

struct Config {

    int size = 128;
    double eps = 1e-6;
    int max_iters = 1000000;
    bool print_grid = false;
};

void print_usage(const char* program) {

    std::cout
        << "Usage: " << program << " [options]\n"
        << "Options:\n"
        << "  --size N        Grid size\n"
        << "  --eps VALUE     Stop accuracy\n"
        << "  --max-iters N   Maximum iterations\n"
        << "  --print         Print resulting grid\n"
        << "  --help          Show help\n";
}

#if TASK6_HAS_BOOST

Config parse_args(int argc, char** argv) {

    namespace po = boost::program_options;

    Config cfg;

    po::options_description desc("Options");

    desc.add_options()
        ("help,h", "show help")
        ("size,n",
            po::value<int>(&cfg.size)->default_value(cfg.size),
            "grid size")
        ("eps,e",
            po::value<double>(&cfg.eps)->default_value(cfg.eps),
            "accuracy")
        ("max-iters,i",
            po::value<int>(&cfg.max_iters)->default_value(cfg.max_iters),
            "maximum iterations")
        ("print,p",
            po::bool_switch(&cfg.print_grid),
            "print resulting grid");

    po::variables_map vm;

    po::store(
        po::parse_command_line(argc, argv, desc),
        vm
    );

    po::notify(vm);

    if (vm.count("help")) {

        std::cout << desc << '\n';

        std::exit(0);
    }

    return cfg;
}

#else

Config parse_args(int argc, char** argv) {

    Config cfg;

    for (int i = 1; i < argc; ++i) {

        const std::string arg = argv[i];

        auto require_value =
            [&](const std::string& name) -> const char* {

            if (i + 1 >= argc) {

                throw std::runtime_error(
                    "Option " + name + " requires a value"
                );
            }

            return argv[++i];
        };

        if (arg == "--help" || arg == "-h") {

            print_usage(argv[0]);

            std::exit(0);
        }

        else if (arg == "--size" || arg == "-n") {

            cfg.size = std::stoi(require_value(arg));
        }

        else if (arg == "--eps" || arg == "-e") {

            cfg.eps = std::stod(require_value(arg));
        }

        else if (arg == "--max-iters" || arg == "-i") {

            cfg.max_iters = std::stoi(require_value(arg));
        }

        else if (arg == "--print" || arg == "-p") {

            cfg.print_grid = true;
        }

        else {

            throw std::runtime_error(
                "Unknown option: " + arg
            );
        }
    }

    return cfg;
}

#endif

void validate_config(const Config& cfg) {

    if (cfg.size < 2) {

        throw std::runtime_error(
            "Grid size must be at least 2"
        );
    }

    if (cfg.eps <= 0.0) {

        throw std::runtime_error(
            "Accuracy must be positive"
        );
    }

    if (cfg.max_iters <= 0) {

        throw std::runtime_error(
            "Maximum iterations must be positive"
        );
    }
}

inline int idx(int row, int col, int size) {

    return row * size + col;
}

void init_boundaries(
    std::vector<double>& grid,
    int size
) {

    const double top_left = 10.0;
    const double top_right = 20.0;
    const double bottom_right = 30.0;
    const double bottom_left = 20.0;

    const double denom =
        static_cast<double>(size - 1);

    std::fill(grid.begin(), grid.end(), 0.0);

    for (int i = 0; i < size; ++i) {

        const double t =
            static_cast<double>(i) / denom;

        // верх
        grid[idx(0, i, size)] =
            top_left +
            (top_right - top_left) * t;

        // низ
        grid[idx(size - 1, i, size)] =
            bottom_left +
            (bottom_right - bottom_left) * t;

        // лево
        grid[idx(i, 0, size)] =
            top_left +
            (bottom_left - top_left) * t;

        // право
        grid[idx(i, size - 1, size)] =
            top_right +
            (bottom_right - top_right) * t;
    }
}

void print_grid(
    const std::vector<double>& grid,
    int size
) {

    std::cout
        << std::fixed
        << std::setprecision(6);

    for (int row = 0; row < size; ++row) {

        for (int col = 0; col < size; ++col) {

            std::cout
                << std::setw(12)
                << grid[idx(row, col, size)];
        }

        std::cout << '\n';
    }
}

struct SolveResult {

    int iterations = 0;
    double error = 0.0;
    double seconds = 0.0;
};

SolveResult solve(
    std::vector<double>& grid,
    int size,
    double eps,
    int max_iters
) {

    const int total = size * size;

    std::vector<double> next = grid;

    double* current = grid.data();
    double* next_grid = next.data();

    SolveResult result;

    auto start =
        std::chrono::high_resolution_clock::now();

#pragma acc data copy(current[0:total], next_grid[0:total])
    {
        for (int iter = 1;
             iter <= max_iters;
             ++iter) {

            double error = 0.0;

#pragma acc parallel loop collapse(2) reduction(max:error)
            for (int row = 1;
                 row < size - 1;
                 ++row) {

                for (int col = 1;
                     col < size - 1;
                     ++col) {

                    const int pos =
                        row * size + col;

                    next_grid[pos] =
                        0.25 * (
                            current[pos - 1] +
                            current[pos + 1] +
                            current[pos - size] +
                            current[pos + size]
                        );

                    const double diff =
                        std::fabs(
                            next_grid[pos] - current[pos]
                        );

                    if (diff > error) {
                        error = diff;
                    }
                }
            }

            // swap ТОЛЬКО указателей
            std::swap(current, next_grid);

            result.iterations = iter;
            result.error = error;

            if (error < eps) {
                break;
            }
        }

#pragma acc update self(current[0:total])
    }

    // если финальные данные лежат в next
    if (current != grid.data()) {

        std::copy(
            current,
            current + total,
            grid.begin()
        );
    }

    auto finish =
        std::chrono::high_resolution_clock::now();

    result.seconds =
        std::chrono::duration<double>(
            finish - start
        ).count();

    return result;
}

int main(int argc, char** argv) {

    try {

        const Config cfg =
            parse_args(argc, argv);

        validate_config(cfg);

        std::vector<double> grid(
            static_cast<size_t>(cfg.size)
            * cfg.size
        );

        init_boundaries(grid, cfg.size);

        const SolveResult result =
            solve(
                grid,
                cfg.size,
                cfg.eps,
                cfg.max_iters
            );

        std::cout
            << "Grid size: "
            << cfg.size
            << "x"
            << cfg.size
            << '\n';

        std::cout
            << "Iterations: "
            << result.iterations
            << '\n';

        std::cout
            << std::scientific
            << "Error: "
            << result.error
            << '\n';

        std::cout
            << std::fixed
            << std::setprecision(6)
            << "Time: "
            << result.seconds
            << " sec\n";

        if (cfg.print_grid || cfg.size <= 13) {

            std::cout << "Grid:\n";

            print_grid(grid, cfg.size);
        }
    }

    catch (const std::exception& ex) {

        std::cerr
            << "Error: "
            << ex.what()
            << '\n';

        print_usage(argv[0]);

        return 1;
    }

    return 0;
}