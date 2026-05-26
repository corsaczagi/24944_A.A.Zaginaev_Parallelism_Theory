#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
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
    bool save_to_file = false;
    std::string output_file = "results.txt";
    int num_threads = 4;
    bool use_openmp = false;
};

void print_usage(const char* program) {
    std::cout
        << "Usage: " << program << " [options]\n"
        << "Options:\n"
        << "  --size N        Grid size: 10, 13, 128, 256, 512, 1024 ...\n"
        << "  --eps VALUE     Stop accuracy, default 1e-6\n"
        << "  --max-iters N   Maximum iterations, default 1000000\n"
        << "  --print         Print resulting grid\n"
        << "  --save FILE     Save results to file\n"
        << "  --openmp        Use OpenMP (for multicore CPU)\n"
        << "  --threads N     Number of threads for OpenMP (default 4)\n"
        << "  --help          Show help\n";
}

#if TASK6_HAS_BOOST
Config parse_args(int argc, char** argv) {
    namespace po = boost::program_options;

    Config cfg;
    std::string output_file;

    po::options_description desc("Options");
    desc.add_options()
        ("help,h", "show help")
        ("size,n", po::value<int>(&cfg.size)->default_value(cfg.size), "grid size")
        ("eps,e", po::value<double>(&cfg.eps)->default_value(cfg.eps), "accuracy")
        ("max-iters,i", po::value<int>(&cfg.max_iters)->default_value(cfg.max_iters), "maximum iterations")
        ("print,p", po::bool_switch(&cfg.print_grid), "print resulting grid")
        ("save,s", po::value<std::string>(&cfg.output_file), "save results to file")
        ("openmp", po::bool_switch(&cfg.use_openmp), "use OpenMP for multicore CPU")
        ("threads,t", po::value<int>(&cfg.num_threads)->default_value(cfg.num_threads), "number of OpenMP threads");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << '\n';
        std::exit(0);
    }

    if (vm.count("save")) {
        cfg.save_to_file = true;
    }

    return cfg;
}
#else
Config parse_args(int argc, char** argv) {
    Config cfg;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        auto require_value = [&](const std::string& name) -> const char* {
            if (i + 1 >= argc) {
                throw std::runtime_error("Option " + name + " requires a value");
            }
            return argv[++i];
        };

        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            std::exit(0);
        } else if (arg == "--size" || arg == "-n") {
            cfg.size = std::stoi(require_value(arg));
        } else if (arg == "--eps" || arg == "-e") {
            cfg.eps = std::stod(require_value(arg));
        } else if (arg == "--max-iters" || arg == "-i") {
            cfg.max_iters = std::stoi(require_value(arg));
        } else if (arg == "--print" || arg == "-p") {
            cfg.print_grid = true;
        } else if (arg == "--save" || arg == "-s") {
            cfg.output_file = require_value(arg);
            cfg.save_to_file = true;
        } else if (arg == "--openmp") {
            cfg.use_openmp = true;
        } else if (arg == "--threads" || arg == "-t") {
            cfg.num_threads = std::stoi(require_value(arg));
        } else {
            throw std::runtime_error("Unknown option: " + arg);
        }
    }

    return cfg;
}
#endif

void validate_config(const Config& cfg) {
    if (cfg.size < 2) {
        throw std::runtime_error("Grid size must be at least 2");
    }
    if (cfg.eps <= 0.0) {
        throw std::runtime_error("Accuracy must be positive");
    }
    if (cfg.max_iters <= 0) {
        throw std::runtime_error("Maximum iterations must be positive");
    }
    if (cfg.num_threads < 1) {
        throw std::runtime_error("Number of threads must be at least 1");
    }
}

int idx(int row, int col, int size) {
    return row * size + col;
}

void init_boundaries(std::vector<double>& grid, int size) {
    const double top_left = 10.0;
    const double top_right = 20.0;
    const double bottom_left = 30.0;
    const double bottom_right = 20.0;
    const double denom = static_cast<double>(size - 1);

    std::fill(grid.begin(), grid.end(), 0.0);

    // Углы
    grid[idx(0, 0, size)] = top_left;
    grid[idx(0, size - 1, size)] = top_right;
    grid[idx(size - 1, 0, size)] = bottom_left;
    grid[idx(size - 1, size - 1, size)] = bottom_right;

    // Верхняя граница (от top_left до top_right)
    for (int i = 1; i < size - 1; ++i) {
        const double t = static_cast<double>(i) / denom;
        grid[idx(0, i, size)] = top_left + (top_right - top_left) * t;
    }

    // Нижняя граница (от bottom_left до bottom_right)
    for (int i = 1; i < size - 1; ++i) {
        const double t = static_cast<double>(i) / denom;
        grid[idx(size - 1, i, size)] = bottom_left + (bottom_right - bottom_left) * t;
    }

    // Левая граница (от top_left до bottom_left)
    for (int i = 1; i < size - 1; ++i) {
        const double t = static_cast<double>(i) / denom;
        grid[idx(i, 0, size)] = top_left + (bottom_left - top_left) * t;
    }

    // Правая граница (константа = top_right)
    for (int i = 1; i < size - 1; ++i) {
        grid[idx(i, size - 1, size)] = top_right;
    }
}

void print_grid(const std::vector<double>& grid, int size, std::ostream& os = std::cout) {
    os << std::fixed << std::setprecision(6);
    for (int row = 0; row < size; ++row) {
        for (int col = 0; col < size; ++col) {
            os << std::setw(12) << grid[idx(row, col, size)];
        }
        os << '\n';
    }
}

struct SolveResult {
    int iterations = 0;
    double error = 0.0;
    double seconds = 0.0;
};

SolveResult solve_openacc(std::vector<double>& grid, int size, double eps, int max_iters) {
    const int total = size * size;
    std::vector<double> next = grid;

    double* current_ptr = grid.data();
    double* next_ptr = next.data();

    SolveResult result;
    auto start = std::chrono::high_resolution_clock::now();

#pragma acc data copy(current_ptr[0:total], next_ptr[0:total])
    {
        for (int iter = 1; iter <= max_iters; ++iter) {
            double error = 0.0;

#pragma acc parallel loop collapse(2) reduction(max:error)
            for (int row = 1; row < size - 1; ++row) {
                for (int col = 1; col < size - 1; ++col) {
                    const int pos = row * size + col;
                    const double value = 0.25 * (
                        current_ptr[pos - 1] +   // слева
                        current_ptr[pos + 1] +   // справа
                        current_ptr[pos - size] + // сверху
                        current_ptr[pos + size]   // снизу
                    );
                    const double diff = std::fabs(value - current_ptr[pos]);
                    if (diff > error) error = diff;
                    next_ptr[pos] = value;
                }
            }

#pragma acc parallel loop collapse(2)
            for (int row = 1; row < size - 1; ++row) {
                for (int col = 1; col < size - 1; ++col) {
                    const int pos = row * size + col;
                    current_ptr[pos] = next_ptr[pos];
                }
            }

            result.iterations = iter;
            result.error = error;

            if (error < eps) {
                break;
            }
        }
    }

    auto finish = std::chrono::high_resolution_clock::now();
    result.seconds = std::chrono::duration<double>(finish - start).count();
    return result;
}

#ifdef _OPENMP
#include <omp.h>

SolveResult solve_omp(std::vector<double>& grid, int size, double eps, int max_iters, int num_threads) {
    omp_set_num_threads(num_threads);
    
    const int total = size * size;
    std::vector<double> next = grid;

    SolveResult result;
    auto start = std::chrono::high_resolution_clock::now();

    for (int iter = 1; iter <= max_iters; ++iter) {
        double error = 0.0;

        #pragma omp parallel for collapse(2) reduction(max:error)
        for (int row = 1; row < size - 1; ++row) {
            for (int col = 1; col < size - 1; ++col) {
                const int pos = row * size + col;
                const double value = 0.25 * (
                    grid[pos - 1] + grid[pos + 1] +
                    grid[pos - size] + grid[pos + size]
                );
                const double diff = std::fabs(value - grid[pos]);
                if (diff > error) error = diff;
                next[pos] = value;
            }
        }

        #pragma omp parallel for collapse(2)
        for (int row = 1; row < size - 1; ++row) {
            for (int col = 1; col < size - 1; ++col) {
                const int pos = row * size + col;
                grid[pos] = next[pos];
            }
        }

        result.iterations = iter;
        result.error = error;

        if (error < eps) {
            break;
        }
    }

    auto finish = std::chrono::high_resolution_clock::now();
    result.seconds = std::chrono::duration<double>(finish - start).count();
    return result;
}
#endif

SolveResult solve_sequential(std::vector<double>& grid, int size, double eps, int max_iters) {
    const int total = size * size;
    std::vector<double> next = grid;

    SolveResult result;
    auto start = std::chrono::high_resolution_clock::now();

    for (int iter = 1; iter <= max_iters; ++iter) {
        double error = 0.0;

        for (int row = 1; row < size - 1; ++row) {
            for (int col = 1; col < size - 1; ++col) {
                const int pos = row * size + col;
                const double value = 0.25 * (
                    grid[pos - 1] + grid[pos + 1] +
                    grid[pos - size] + grid[pos + size]
                );
                const double diff = std::fabs(value - grid[pos]);
                if (diff > error) error = diff;
                next[pos] = value;
            }
        }

        for (int row = 1; row < size - 1; ++row) {
            for (int col = 1; col < size - 1; ++col) {
                const int pos = row * size + col;
                grid[pos] = next[pos];
            }
        }

        result.iterations = iter;
        result.error = error;

        if (error < eps) {
            break;
        }
    }

    auto finish = std::chrono::high_resolution_clock::now();
    result.seconds = std::chrono::duration<double>(finish - start).count();
    return result;
}

void save_to_file(const std::string& filename, const Config& cfg, const SolveResult& result) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Warning: cannot open file " << filename << std::endl;
        return;
    }

    file << "========================================\n";
    file << "Уравнение теплопроводности (метод Якоби)\n";
    file << "========================================\n\n";
    file << "Параметры:\n";
    file << "  Размер сетки: " << cfg.size << "x" << cfg.size << "\n";
    file << "  Точность: " << std::scientific << cfg.eps << "\n";
    file << "  Макс. итераций: " << cfg.max_iters << "\n";
    file << "  Режим: " << (cfg.use_openmp ? "OpenMP (CPU многоядерный)" : "OpenACC (GPU/CPU)") << "\n";
    if (cfg.use_openmp) {
        file << "  Потоков: " << cfg.num_threads << "\n";
    }
    file << "\nРезультаты:\n";
    file << "  Итераций: " << result.iterations << "\n";
    file << "  Ошибка: " << std::scientific << result.error << "\n";
    file << "  Время: " << std::fixed << std::setprecision(6) << result.seconds << " сек\n";
    file.close();
    
    std::cout << "Результаты сохранены в " << filename << std::endl;
}

int main(int argc, char** argv) {
    try {
        Config cfg = parse_args(argc, argv);
        validate_config(cfg);

        std::vector<double> grid(static_cast<size_t>(cfg.size) * cfg.size);
        init_boundaries(grid, cfg.size);

        // Вывод информации о запуске
        std::cout << "========================================\n";
        std::cout << "Уравнение теплопроводности (метод Якоби)\n";
        std::cout << "========================================\n\n";
        std::cout << "Размер сетки: " << cfg.size << "x" << cfg.size << "\n";
        std::cout << "Точность: " << std::scientific << cfg.eps << "\n";
        std::cout << "Макс. итераций: " << cfg.max_iters << "\n";

        SolveResult result;

#ifdef _OPENMP
        if (cfg.use_openmp) {
            std::cout << "Режим: OpenMP (CPU многоядерный)\n";
            std::cout << "Потоков: " << cfg.num_threads << "\n";
            result = solve_omp(grid, cfg.size, cfg.eps, cfg.max_iters, cfg.num_threads);
        } else {
            std::cout << "Режим: OpenACC (GPU/CPU)\n";
            result = solve_openacc(grid, cfg.size, cfg.eps, cfg.max_iters);
        }
#else
        if (cfg.use_openmp) {
            std::cout << "Предупреждение: OpenMP не поддерживается, используем последовательную версию\n";
        }
        std::cout << "Режим: последовательный\n";
        result = solve_sequential(grid, cfg.size, cfg.eps, cfg.max_iters);
#endif

        std::cout << "\nРезультаты:\n";
        std::cout << "  Итераций: " << result.iterations << '\n';
        std::cout << "  Ошибка: " << std::scientific << result.error << '\n';
        std::cout << "  Время: " << std::fixed << std::setprecision(6)
                  << result.seconds << " сек\n";

        if (cfg.print_grid || cfg.size <= 13) {
            std::cout << "\nФинальная сетка:\n";
            print_grid(grid, cfg.size);
        }

        if (cfg.save_to_file) {
            save_to_file(cfg.output_file, cfg, result);
        }

    } catch (const std::exception& ex) {
        std::cerr << "Ошибка: " << ex.what() << '\n';
        print_usage(argv[0]);
        return 1;
    }

    return 0;
}