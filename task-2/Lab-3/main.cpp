#include <iostream>
#include <fstream>
#include <cmath>
#include <omp.h>
#include <vector>

using namespace std;

// Функция для умножения матрицы на вектор
void matvec(const vector<double>& A, const vector<double>& x, 
            vector<double>& Ax, int N) {
    for (int i = 0; i < N; i++) {
        double sum = 0.0;
        for (int j = 0; j < N; j++) {
            sum += A[i * N + j] * x[j];
        }
        Ax[i] = sum;
    }
}

// Вариант 1: #pragma omp parallel for для каждого цикла
double solve_iterative_parallel_v1(const vector<double>& A, const vector<double>& b,
                                     vector<double>& x, int N, double tau, 
                                     double eps, int max_iter, double& exec_time) {
    vector<double> Ax(N, 0.0);
    vector<double> x_new(N, 0.0);
    double diff = 0.0;
    int iter = 0;
    
    double start = omp_get_wtime();
    
    for (iter = 0; iter < max_iter; iter++) {
        // Вычисляем Ax
        #pragma omp parallel for
        for (int i = 0; i < N; i++) {
            double sum = 0.0;
            for (int j = 0; j < N; j++) {
                sum += A[i * N + j] * x[j];
            }
            Ax[i] = sum;
        }
        
        double local_diff = 0.0;
        
        #pragma omp parallel for reduction(max:local_diff)
        for (int i = 0; i < N; i++) {
            x_new[i] = x[i] - tau * (Ax[i] - b[i]);
            double d = fabs(x_new[i] - x[i]);
            if (d > local_diff) local_diff = d;
        }
        
        #pragma omp parallel for
        for (int i = 0; i < N; i++) {
            x[i] = x_new[i];
        }
        
        diff = local_diff;
        if (diff < eps) break;
    }
    
    double end = omp_get_wtime();
    exec_time = end - start;
    
    return diff;
}

// Вариант 2: одна #pragma omp parallel охватывает весь алгоритм
double solve_iterative_parallel_v2(const vector<double>& A, const vector<double>& b,
                                     vector<double>& x, int N, double tau, 
                                     double eps, int max_iter, double& exec_time) {
    vector<double> Ax(N, 0.0);
    vector<double> x_new(N, 0.0);
    double diff = 0.0;
    int iter = 0;
    
    double start = omp_get_wtime();
    
    #pragma omp parallel
    {
        for (iter = 0; iter < max_iter; iter++) {
            #pragma omp for
            for (int i = 0; i < N; i++) {
                double sum = 0.0;
                for (int j = 0; j < N; j++) {
                    sum += A[i * N + j] * x[j];
                }
                Ax[i] = sum;
            }
            
            double local_diff = 0.0;  
            
            #pragma omp for
            for (int i = 0; i < N; i++) {
                x_new[i] = x[i] - tau * (Ax[i] - b[i]);
                double d = fabs(x_new[i] - x[i]);
                if (d > local_diff) local_diff = d;
            }
            
            #pragma omp for
            for (int i = 0; i < N; i++) {
                x[i] = x_new[i];
            }
            
            #pragma omp critical
            {
                if (local_diff > diff) diff = local_diff;
            }
            
            #pragma omp barrier
            #pragma omp single
            {
                if (diff < eps) iter = max_iter;  
            }
        }
    }
    
    double end = omp_get_wtime();
    exec_time = end - start;
    
    return diff;
}

// Последовательная версия для сравнения
double solve_iterative_serial(const vector<double>& A, const vector<double>& b,
                               vector<double>& x, int N, double tau, 
                               double eps, int max_iter, double& exec_time) {
    vector<double> Ax(N, 0.0);
    vector<double> x_new(N, 0.0);
    double diff = 0.0;
    int iter = 0;
    
    double start = omp_get_wtime();
    
    for (iter = 0; iter < max_iter; iter++) {
        for (int i = 0; i < N; i++) {
            double sum = 0.0;
            for (int j = 0; j < N; j++) {
                sum += A[i * N + j] * x[j];
            }
            Ax[i] = sum;
        }
        
        diff = 0.0;
        for (int i = 0; i < N; i++) {
            x_new[i] = x[i] - tau * (Ax[i] - b[i]);
            double d = fabs(x_new[i] - x[i]);
            if (d > diff) diff = d;
        }
        
        for (int i = 0; i < N; i++) {
            x[i] = x_new[i];
        }
        
        if (diff < eps) break;
    }
    
    double end = omp_get_wtime();
    exec_time = end - start;
    
    return diff;
}

int main() {
    int N = 5000; 
    double tau = 0.1; 
    double eps = 1e-6; 
    int max_iter = 10000;  
    
    vector<int> threads = {1, 2, 4, 7, 8, 16, 20, 40};
    
    cout << "Решение СЛАУ методом простой итерации" << endl;
    cout << "Размер матрицы: " << N << " x " << N << endl;
    cout << "================================================" << endl;
    
    vector<double> A(N * N, 1.0);
    vector<double> b(N, N + 1.0);
    vector<double> x(N, 0.0);
    
    for (int i = 0; i < N; i++) {
        A[i * N + i] = 2.0;
    }
    
    ofstream outfile("slau_benchmark.txt");
    outfile << "# Потоков Время_V1(сек) Время_V2(сек) Ускорение_V1 Ускорение_V2" << endl;
    
    // Замеры для разного количества потоков
    for (int num_threads : threads) {
        omp_set_num_threads(num_threads);
        
        vector<double> x1(N, 0.0);
        double time1, time2;
        
        solve_iterative_parallel_v1(A, b, x1, N, tau, eps, max_iter, time1);
        
        fill(x1.begin(), x1.end(), 0.0);
        double diff1 = solve_iterative_parallel_v1(A, b, x1, N, tau, eps, max_iter, time1);
        
        // Тест варианта 2
        vector<double> x2(N, 0.0);
        fill(x2.begin(), x2.end(), 0.0);
        double diff2 = solve_iterative_parallel_v2(A, b, x2, N, tau, eps, max_iter, time2);
        
        cout << "Потоков: " << num_threads << endl;
        cout << "  V1: время = " << time1 << " сек, diff = " << diff1 << endl;
        cout << "  V2: время = " << time2 << " сек, diff = " << diff2 << endl;
        
        static double base_time1 = 0.0, base_time2 = 0.0;
        if (num_threads == 1) {
            base_time1 = time1;
            base_time2 = time2;
        }
        
        double speedup1 = base_time1 / time1;
        double speedup2 = base_time2 / time2;
        
        outfile << num_threads << " " << time1 << " " << time2 << " " 
                << speedup1 << " " << speedup2 << endl;
        
        cout << "  Ускорение V1: " << speedup1 << "x, V2: " << speedup2 << "x" << endl;
        cout << "----------------------------------------" << endl;
    }
    
    outfile.close();
    
    // Исследование schedule (для 8 потоков, фиксированный размер)
    cout << "\n\nИсследование параметров schedule" << endl;
    cout << "================================================" << endl;
    
    vector<int> threads_fixed = {8};
    vector<string> schedules = {"static", "dynamic", "guided", "auto"};
    
    ofstream schedule_file("schedule_benchmark.txt");
    schedule_file << "# schedule_type время(сек)" << endl;
    
    for (int num_threads : threads_fixed) {
        omp_set_num_threads(num_threads);
        vector<double> x_test(N, 0.0);
        
        for (const string& sched : schedules) {
            double time = 0.0;
            
            // Код с разными schedule
            vector<double> Ax(N, 0.0);
            vector<double> x_new(N, 0.0);
            double diff = 0.0;
            int iter = 0;
            
            double start = omp_get_wtime();
            
            for (iter = 0; iter < max_iter && diff < eps; iter++) {
                if (sched == "static") {
                    #pragma omp parallel for schedule(static)
                    for (int i = 0; i < N; i++) {
                        double sum = 0.0;
                        for (int j = 0; j < N; j++) {
                            sum += A[i * N + j] * x_test[j];
                        }
                        Ax[i] = sum;
                    }
                } else if (sched == "dynamic") {
                    #pragma omp parallel for schedule(dynamic, 64)
                    for (int i = 0; i < N; i++) {
                        double sum = 0.0;
                        for (int j = 0; j < N; j++) {
                            sum += A[i * N + j] * x_test[j];
                        }
                        Ax[i] = sum;
                    }
                } else if (sched == "guided") {
                    #pragma omp parallel for schedule(guided)
                    for (int i = 0; i < N; i++) {
                        double sum = 0.0;
                        for (int j = 0; j < N; j++) {
                            sum += A[i * N + j] * x_test[j];
                        }
                        Ax[i] = sum;
                    }
                } else {
                    #pragma omp parallel for schedule(auto)
                    for (int i = 0; i < N; i++) {
                        double sum = 0.0;
                        for (int j = 0; j < N; j++) {
                            sum += A[i * N + j] * x_test[j];
                        }
                        Ax[i] = sum;
                    }
                }
                
                #pragma omp parallel for reduction(max:diff)
                for (int i = 0; i < N; i++) {
                    x_new[i] = x_test[i] - tau * (Ax[i] - b[i]);
                    double d = fabs(x_new[i] - x_test[i]);
                    if (d > diff) diff = d;
                }
                
                #pragma omp parallel for
                for (int i = 0; i < N; i++) {
                    x_test[i] = x_new[i];
                }
            }
            
            double end = omp_get_wtime();
            time = end - start;
            
            cout << "Schedule: " << sched << " | Время: " << time << " сек" << endl;
            schedule_file << sched << " " << time << endl;
        }
    }
    
    schedule_file.close();
    
    cout << "Результаты сохранены в slau_benchmark.txt и schedule_benchmark.txt" << endl;
    
    return 0;
}