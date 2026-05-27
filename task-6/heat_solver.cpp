#include <iostream>
#include <iomanip>
#include <cmath>
#include <cstring>
#include <chrono>
#include <cstdlib>
#include <algorithm>

class HeatEquationSolver {
private:
    int nx, ny;           // Размеры сетки
    double precision;     // Точность
    int max_iterations;   // Максимальное число итераций
    double* grid;         // Текущая сетка
    double* newgrid;      // Новая сетка
    double* grid_host;    // Копия для CPU
    
public:
    HeatEquationSolver(int n, double eps, int max_iter) 
        : nx(n), ny(n), precision(eps), max_iterations(max_iter) {
        
        size_t size = nx * ny * sizeof(double);
        grid = new double[nx * ny]();
        newgrid = new double[nx * ny]();
        grid_host = new double[nx * ny]();
        
        initBoundaryConditions();
    }
    
    ~HeatEquationSolver() {
        delete[] grid;
        delete[] newgrid;
        delete[] grid_host;
    }
    
    // Инициализация граничных условий (линейная интерполяция между углами)
    void initBoundaryConditions() {
        double corners[4] = {10.0, 20.0, 30.0, 20.0};
        
        // Обнуляем все
        memset(grid, 0, nx * ny * sizeof(double));
        
        // Нижняя граница (y = 0)
        for (int i = 0; i < nx; i++) {
            double t = static_cast<double>(i) / (nx - 1);
            grid[i] = corners[0] * (1 - t) + corners[1] * t;
        }
        
        // Верхняя граница (y = ny - 1)
        for (int i = 0; i < nx; i++) {
            double t = static_cast<double>(i) / (nx - 1);
            grid[i + (ny - 1) * nx] = corners[3] * (1 - t) + corners[2] * t;
        }
        
        // Левая граница (x = 0)
        for (int j = 0; j < ny; j++) {
            double t = static_cast<double>(j) / (ny - 1);
            grid[j * nx] = corners[0] * (1 - t) + corners[3] * t;
        }
        
        // Правая граница (x = nx - 1)
        for (int j = 0; j < ny; j++) {
            double t = static_cast<double>(j) / (ny - 1);
            grid[(nx - 1) + j * nx] = corners[1] * (1 - t) + corners[2] * t;
        }
        
        // Копируем границы в новую сетку
        memcpy(newgrid, grid, nx * ny * sizeof(double));
        memcpy(grid_host, grid, nx * ny * sizeof(double));
    }
    
    // GPU версия с OpenACC
    int solveGPU() {
        int iteration = 0;
        double error = 0.0;
        int size = nx * ny;
        
        // Копируем данные на GPU
        #pragma acc data copyin(grid[0:size], newgrid[0:size])
        {
            for (iteration = 0; iteration < max_iterations; iteration++) {
                error = 0.0;
                
                // Вычисляем новые значения (метод Якоби)
                // Используем collapse(2) для лучшего распараллеливания
                #pragma acc parallel loop collapse(2) reduction(max:error) copy(error)
                for (int j = 1; j < ny - 1; j++) {
                    for (int i = 1; i < nx - 1; i++) {
                        int idx = j * nx + i;
                        // Схема "крест": новое значение = среднее из 4 соседей
                        double new_val = (grid[idx - 1] + grid[idx + 1] + 
                                         grid[idx - nx] + grid[idx + nx]) * 0.25;
                        newgrid[idx] = new_val;
                        
                        double diff = fabs(new_val - grid[idx]);
                        if (diff > error) error = diff;
                    }
                }
                
                // Обновляем сетку
                #pragma acc parallel loop
                for (int i = 1; i < size - 1; i++) {
                    if (i % nx != 0 && i % nx != nx - 1) {
                        grid[i] = newgrid[i];
                    }
                }
                
                // Проверка сходимости
                if (error < precision) {
                    break;
                }
            }
        }
        
        return iteration;
    }
    
    // CPU версия
    int solveCPU() {
        int iteration = 0;
        double error = 0.0;
        int size = nx * ny;
        
        // Копируем начальные условия
        memcpy(grid, grid_host, size * sizeof(double));
        memcpy(newgrid, grid_host, size * sizeof(double));
        
        for (iteration = 0; iteration < max_iterations; iteration++) {
            error = 0.0;
            
            // Вычисляем новые значения
            for (int j = 1; j < ny - 1; j++) {
                for (int i = 1; i < nx - 1; i++) {
                    int idx = j * nx + i;
                    double new_val = (grid[idx - 1] + grid[idx + 1] + 
                                     grid[idx - nx] + grid[idx + nx]) * 0.25;
                    newgrid[idx] = new_val;
                    
                    double diff = fabs(new_val - grid[idx]);
                    if (diff > error) error = diff;
                }
            }
            
            // Обновляем сетку
            memcpy(grid, newgrid, size * sizeof(double));
            
            if (error < precision) {
                break;
            }
        }
        
        return iteration;
    }
    
    // Вывод сетки
    void printGrid(int size) {
        std::cout << std::fixed << std::setprecision(4);
        for (int j = 0; j < size; j++) {
            for (int i = 0; i < size; i++) {
                std::cout << std::setw(8) << grid[j * nx + i];
            }
            std::cout << std::endl;
        }
    }
    
    // Получение указателя на данные
    double* getGrid() { return grid; }
};

int main(int argc, char* argv[]) {
    int grid_size = 512;
    double precision = 1e-6;
    int max_iterations = 1000000;
    bool print_matrix = false;
    bool use_gpu = true;
    
    // Парсинг аргументов командной строки
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--size") == 0 && i+1 < argc) {
            grid_size = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--precision") == 0 && i+1 < argc) {
            precision = atof(argv[++i]);
        } else if (strcmp(argv[i], "--iterations") == 0 && i+1 < argc) {
            max_iterations = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--print") == 0) {
            print_matrix = true;
        } else if (strcmp(argv[i], "--cpu") == 0) {
            use_gpu = false;
        } else if (strcmp(argv[i], "--help") == 0) {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "  --size N        Grid size NxN (128, 256, 512, 1024)" << std::endl;
            std::cout << "  --precision EPS Precision (default: 1e-6)" << std::endl;
            std::cout << "  --iterations N  Max iterations (default: 1000000)" << std::endl;
            std::cout << "  --print         Print 10x10 and 13x13 results" << std::endl;
            std::cout << "  --cpu           Use CPU instead of GPU" << std::endl;
            return 0;
        }
    }
    
    std::cout << "========================================" << std::endl;
    std::cout << "Heat Equation Solver (2D Laplace)" << std::endl;
    std::cout << "Jacobi Iteration Method" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Grid size: " << grid_size << "x" << grid_size << std::endl;
    std::cout << "Precision: " << precision << std::endl;
    std::cout << "Max iterations: " << max_iterations << std::endl;
    std::cout << "Device: " << (use_gpu ? "GPU (OpenACC)" : "CPU") << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    // Вывод матриц 10x10 и 13x13 для проверки
    if (print_matrix) {
        std::cout << "\n=== 10x10 Grid ===" << std::endl;
        HeatEquationSolver solver10(10, precision, max_iterations);
        if (use_gpu) {
            solver10.solveGPU();
        } else {
            solver10.solveCPU();
        }
        solver10.printGrid(10);
        
        std::cout << "\n=== 13x13 Grid ===" << std::endl;
        HeatEquationSolver solver13(13, precision, max_iterations);
        if (use_gpu) {
            solver13.solveGPU();
        } else {
            solver13.solveCPU();
        }
        solver13.printGrid(13);
        
        std::cout << std::endl;
        return 0;
    }
    
    // Основной расчет
    HeatEquationSolver solver(grid_size, precision, max_iterations);
    
    auto start = std::chrono::high_resolution_clock::now();
    int iterations;
    if (use_gpu) {
        iterations = solver.solveGPU();
    } else {
        iterations = solver.solveCPU();
    }
    auto end = std::chrono::high_resolution_clock::now();
    double time = std::chrono::duration<double>(end - start).count();
    
    std::cout << "\nResults:" << std::endl;
    std::cout << "  Iterations: " << iterations << std::endl;
    std::cout << "  Time: " << time << " seconds" << std::endl;
    std::cout << "  Performance: " << (grid_size * grid_size * iterations / time / 1e6) << " M update/s" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}