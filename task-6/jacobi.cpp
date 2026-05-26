#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <chrono>
#include <cstdlib>
#include <cstring>

#define IND(i, j) ((i) * N + (j))

void print_grid(const std::vector<double>& u, int N) {
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            std::cout << std::fixed << std::setprecision(4) << u[IND(i, j)] << " ";
        }
        std::cout << std::endl;
    }
}

void init_boundary(std::vector<double>& u, int N) {
    // Углы
    u[IND(0, 0)] = 10.0;
    u[IND(0, N-1)] = 20.0;
    u[IND(N-1, 0)] = 30.0;
    u[IND(N-1, N-1)] = 20.0;
    
    // Верхняя граница
    for (int j = 1; j < N - 1; ++j) {
        u[IND(0, j)] = 10.0 + (20.0 - 10.0) * j / (N - 1);
    }
    
    // Нижняя граница
    for (int j = 1; j < N - 1; ++j) {
        u[IND(N-1, j)] = 30.0 + (20.0 - 30.0) * j / (N - 1);
    }
    
    // Левая граница
    for (int i = 1; i < N - 1; ++i) {
        u[IND(i, 0)] = 10.0 + (30.0 - 10.0) * i / (N - 1);
    }
    
    // Правая граница
    for (int i = 1; i < N - 1; ++i) {
        u[IND(i, N-1)] = 20.0;
    }
}

void jacobi_openacc(int N, double eps, int max_iter, int& iter_count, double& final_error) {
    std::vector<double> u(N * N, 0.0);
    std::vector<double> u_new(N * N, 0.0);
    
    init_boundary(u, N);
    
    // Копируем границы в u_new
    #pragma acc parallel loop
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            u_new[IND(i, j)] = u[IND(i, j)];
        }
    }
    
    double error = 1.0;
    int iter = 0;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    while (error > eps && iter < max_iter) {
        // Вычисление новых значений (пятиточечный шаблон)
        #pragma acc parallel loop collapse(2)
        for (int i = 1; i < N - 1; ++i) {
            for (int j = 1; j < N - 1; ++j) {
                u_new[IND(i, j)] = (u[IND(i-1, j)] + u[IND(i+1, j)] +
                                    u[IND(i, j-1)] + u[IND(i, j+1)]) * 0.25;
            }
        }
        
        error = 0.0;
        
        // Вычисление максимальной ошибки и обновление
        #pragma acc parallel loop collapse(2) reduction(max:error)
        for (int i = 1; i < N - 1; ++i) {
            for (int j = 1; j < N - 1; ++j) {
                double diff = std::fabs(u_new[IND(i, j)] - u[IND(i, j)]);
                if (diff > error) error = diff;
                u[IND(i, j)] = u_new[IND(i, j)];
            }
        }
        
        iter++;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    std::cout << "OpenACC: " << elapsed / 1000.0 << " sec" << std::endl;
    
    iter_count = iter;
    final_error = error;
}

int main(int argc, char* argv[]) {
    int N = 128;
    double eps = 1e-6;
    int max_iter = 1000000;
    bool print = false;
    
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            N = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-e") == 0 && i + 1 < argc) {
            eps = atof(argv[++i]);
        } else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            max_iter = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-p") == 0) {
            print = true;
        } else if (strcmp(argv[i], "-h") == 0) {
            std::cout << "Usage: " << argv[0] << " -s N -e eps -m iter" << std::endl;
            return 0;
        }
    }
    
    std::cout << "=== Jacobi 2D (OpenACC) ===" << std::endl;
    std::cout << "Size: " << N << "x" << N << std::endl;
    std::cout << "Eps: " << eps << std::endl;
    std::cout << "Max iter: " << max_iter << std::endl;
    
    int iter_count = 0;
    double final_error = 0.0;
    
    jacobi_openacc(N, eps, max_iter, iter_count, final_error);
    
    std::cout << "Iterations: " << iter_count << std::endl;
    std::cout << "Error: " << std::scientific << final_error << std::endl;
    
    if (print && (N == 10 || N == 13)) {
        std::vector<double> u(N * N, 0.0);
        init_boundary(u, N);
        print_grid(u, N);
    }
    
    return 0;
}