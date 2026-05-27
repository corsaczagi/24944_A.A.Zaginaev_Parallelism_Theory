#include <iostream>
#include <iomanip>
#include <cmath>
#include <cstdlib>
#include <chrono>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

class HeatEquationSolver {
private:
    int nx, ny;
    double precision;
    int max_iterations;
    double* u;      // текущее решение
    double* u_new;  // новое решение
    double* u_host; // для результатов на CPU
    
public:
    HeatEquationSolver(int n, double eps, int max_iter) 
        : nx(n), ny(n), precision(eps), max_iterations(max_iter) {
        
        // Выделение памяти (выровнено для лучшей производительности)
        u = new double[nx * ny]();
        u_new = new double[nx * ny]();
        u_host = new double[nx * ny]();
        
        initBoundaryConditions();
    }
    
    ~HeatEquationSolver() {
        delete[] u;
        delete[] u_new;
        delete[] u_host;
    }
    
    void initBoundaryConditions() {
        // Угловые значения: 10, 20, 30, 20
        double corners[4] = {10.0, 20.0, 30.0, 20.0};
        
        // Установка граничных условий (линейная интерполяция между углами)
        for (int i = 0; i < nx; i++) {
            double t = static_cast<double>(i) / (nx - 1);
            // Нижняя граница (y = 0)
            u[i] = corners[0] * (1 - t) + corners[1] * t;
            // Верхняя граница (y = ny-1)
            u[i + (ny-1) * nx] = corners[3] * (1 - t) + corners[2] * t;
        }
        
        for (int j = 0; j < ny; j++) {
            double t = static_cast<double>(j) / (ny - 1);
            // Левая граница (x = 0)
            u[j * nx] = corners[0] * (1 - t) + corners[3] * t;
            // Правая граница (x = nx-1)
            u[(nx-1) + j * nx] = corners[1] * (1 - t) + corners[2] * t;
        }
        
        // Копирование граничных условий в u_new
        memcpy(u_new, u, nx * ny * sizeof(double));
    }
    
    // Ядро метода Якоби для решения уравнения теплопроводности
    // Уравнение: ∂²u/∂x² + ∂²u/∂y² = 0
    // Разностная схема: u_new[i,j] = (u[i-1,j] + u[i+1,j] + u[i,j-1] + u[i,j+1]) / 4
    
    int solve() {
        int iteration = 0;
        double error = 0.0;
        
        #pragma acc data copyin(u[0:nx*ny]), copy(u_new[0:nx*ny]), create(error)
        {
            for (iteration = 0; iteration < max_iterations; iteration++) {
                error = 0.0;
                
                // Итерация метода Якоби
                #pragma acc parallel loop reduction(max:error) present(u, u_new)
                for (int j = 1; j < ny - 1; j++) {
                    #pragma acc loop vector reduction(max:error)
                    for (int i = 1; i < nx - 1; i++) {
                        int idx = j * nx + i;
                        double new_val = (u[idx - 1] + u[idx + 1] + 
                                         u[idx - nx] + u[idx + nx]) * 0.25;
                        u_new[idx] = new_val;
                        
                        double diff = fabs(new_val - u[idx]);
                        if (diff > error) error = diff;
                    }
                }
                
                // Обмен массивами
                #pragma acc parallel loop present(u, u_new)
                for (int i = 1; i < nx * ny - 1; i++) {
                    u[i] = u_new[i];
                }
                
                // Проверка сходимости
                if (error < precision) {
                    break;
                }
            }
        }
        
        return iteration;
    }
    
    // Версия для CPU (без OpenACC)
    int solveCPU() {
        // Копируем начальные условия
        memcpy(u, u_host, nx * ny * sizeof(double));
        memcpy(u_new, u_host, nx * ny * sizeof(double));
        
        int iteration = 0;
        double error = 0.0;
        
        for (iteration = 0; iteration < max_iterations; iteration++) {
            error = 0.0;
            
            for (int j = 1; j < ny - 1; j++) {
                for (int i = 1; i < nx - 1; i++) {
                    int idx = j * nx + i;
                    double new_val = (u[idx - 1] + u[idx + 1] + 
                                     u[idx - nx] + u[idx + nx]) * 0.25;
                    u_new[idx] = new_val;
                    
                    double diff = fabs(new_val - u[idx]);
                    if (diff > error) error = diff;
                }
            }
            
            // Обмен массивами
            memcpy(u, u_new, nx * ny * sizeof(double));
            
            if (error < precision) {
                break;
            }
        }
        
        return iteration;
    }
    
    void printGrid(int size) {
        std::cout << std::fixed << std::setprecision(4);
        for (int j = 0; j < size; j++) {
            for (int i = 0; i < size; i++) {
                std::cout << std::setw(8) << u[j * nx + i];
            }
            std::cout << std::endl;
        }
    }
    
    void printResults(int iterations, double time, double error) {
        std::cout << "Iterations: " << iterations << std::endl;
        std::cout << "Final error: " << std::scientific << error << std::endl;
        std::cout << "Time: " << time << " seconds" << std::endl;
    }
};

int main(int argc, char* argv[]) {
    int grid_size = 512;
    double precision = 1e-6;
    int max_iterations = 1000000;
    bool print_matrix = false;
    
    try {
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help", "produce help message")
            ("size,s", po::value<int>(&grid_size), "grid size (NxN)")
            ("precision,p", po::value<double>(&precision), "precision (1e-6 default)")
            ("iterations,i", po::value<int>(&max_iterations), "max iterations (1e6 default)")
            ("print", "print result grid for 10x10 and 13x13");
        
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
        
        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 1;
        }
        
        if (vm.count("print")) {
            print_matrix = true;
        }
    }
    catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    // Создание решателя
    HeatEquationSolver solver(grid_size, precision, max_iterations);
    
    // Замер времени
    auto start = std::chrono::high_resolution_clock::now();
    
    // Решение
    int iterations = solver.solve();
    
    auto end = std::chrono::high_resolution_clock::now();
    double time = std::chrono::duration<double>(end - start).count();
    
    // Вывод результатов
    solver.printResults(iterations, time, precision);
    
    // Вывод матриц для проверки
    if (print_matrix) {
        std::cout << "\n=== 10x10 Grid ===" << std::endl;
        HeatEquationSolver solver10(10, precision, max_iterations);
        solver10.solve();
        solver10.printGrid(10);
        
        std::cout << "\n=== 13x13 Grid ===" << std::endl;
        HeatEquationSolver solver13(13, precision, max_iterations);
        solver13.solve();
        solver13.printGrid(13);
    }
    
    return 0;
}