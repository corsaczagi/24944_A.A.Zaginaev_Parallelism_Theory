#include <iostream>
#include <omp.h>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <algorithm>

using namespace std;

// Последовательная версия умножения матрицы на вектор
void matrix_vector_mult_serial(const double* A, const double* x, double* y, int n) {
    for (int i = 0; i < n; i++) {
        double sum = 0.0;
        for (int j = 0; j < n; j++) {
            sum += A[i * n + j] * x[j];
        }
        y[i] = sum;
    }
}

// Параллельная инициализация матрицы с указанным количеством потоков
void init_matrix_omp(double* A, int n, int num_threads) {
    omp_set_num_threads(num_threads);
    #pragma omp parallel
    {
        int nthreads = omp_get_num_threads();
        int threadid = omp_get_thread_num();
        int rows_per_thread = n / nthreads;
        int remainder = n % nthreads;
        
        int start = threadid * rows_per_thread + min(threadid, remainder);
        int end = start + rows_per_thread + (threadid < remainder ? 1 : 0);
        
        for (int i = start; i < end; i++) {
            for (int j = 0; j < n; j++) {
                A[i * n + j] = i + j;  // Простая инициализация без тяжелых вычислений
            }
        }
    }
}

// Параллельная инициализация вектора с указанным количеством потоков
void init_vector_omp(double* x, int n, int num_threads) {
    omp_set_num_threads(num_threads);
    #pragma omp parallel
    {
        int nthreads = omp_get_num_threads();
        int threadid = omp_get_thread_num();
        int elems_per_thread = n / nthreads;
        int remainder = n % nthreads;
        
        int start = threadid * elems_per_thread + min(threadid, remainder);
        int end = start + elems_per_thread + (threadid < remainder ? 1 : 0);
        
        for (int i = start; i < end; i++) {
            x[i] = i;  // Простая инициализация
        }
    }
}

// Параллельная версия умножения матрицы на вектор
void matrix_vector_mult_omp(const double* A, const double* x, double* y, int n, int num_threads) {
    omp_set_num_threads(num_threads);
    #pragma omp parallel
    {
        int nthreads = omp_get_num_threads();
        int threadid = omp_get_thread_num();
        int rows_per_thread = n / nthreads;
        int remainder = n % nthreads;
        
        int start = threadid * rows_per_thread + min(threadid, remainder);
        int end = start + rows_per_thread + (threadid < remainder ? 1 : 0);
        
        for (int i = start; i < end; i++) {
            double sum = 0.0;
            for (int j = 0; j < n; j++) {
                sum += A[i * n + j] * x[j];
            }
            y[i] = sum;
        }
    }
}

// Верификация результатов
bool verify_results(const double* y_parallel, const double* y_serial, int n) {
    double max_diff = 0.0;
    double tolerance = 1e-10;
    int max_diff_idx = -1;
    
    for (int i = 0; i < n; i++) {
        double diff = std::abs(y_parallel[i] - y_serial[i]);
        if (diff > max_diff) {
            max_diff = diff;
            max_diff_idx = i;
        }
        if (diff > tolerance) {
            std::cerr << "Verification failed at index " << i 
                      << ": parallel = " << y_parallel[i] 
                      << ", serial = " << y_serial[i] 
                      << ", diff = " << diff << std::endl;
            return false;
        }
    }
    
    std::cout << "Verification passed: max_diff = " << max_diff 
              << " at index " << max_diff_idx << std::endl;
    return true;
}

int main() {
    // Информация о вычислительном узле
    cout << "\n=== System Information ===" << endl;
    system("lscpu | grep -E 'Model name|CPU\\(s\\)|Thread\\(s\\) per core|Core\\(s\\) per socket|Socket\\(s\\)|L3 cache'");
    system("cat /sys/devices/virtual/dmi/id/product_name 2>/dev/null || echo 'Product name: Not available'");
    system("numactl --hardware 2>/dev/null | grep -A 10 'available' || echo 'numactl not installed'");
    system("free -h | grep Mem");
    system("cat /etc/os-release | grep -E 'PRETTY_NAME|VERSION' | head -2");
    
    // Настройка OpenMP для лучшей производительности на NUMA
    omp_set_nested(0);
    omp_set_dynamic(0);
    setenv("OMP_PROC_BIND", "close", 1);
    setenv("OMP_PLACES", "cores", 1);
    
    cout << "\nOpenMP settings: OMP_PROC_BIND=close, OMP_PLACES=cores" << endl;
    
    int sizes[] = {20000, 40000};
    int threads_list[] = {1, 2, 4, 7, 8, 16, 20, 40};
    int num_sizes = 2;
    int num_threads_list = 8;
    
    const int warmup_iterations = 2;      // Прогрев кэша
    const int measurement_iterations = 5; // Количество измерений
    
    ofstream data_file("benchmark_data.txt");
    if (!data_file.is_open()) {
        cerr << "Error: Cannot open benchmark_data.txt for writing" << endl;
        return 1;
    }
    
    data_file << "# size_matrix threads time_seconds speedup\n";
    
    for (int s = 0; s < num_sizes; s++) {
        int n = sizes[s];
        long long matrix_size = (long long)n * n;
        long long memory_mb = (matrix_size * sizeof(double) + n * sizeof(double) * 2) / (1024 * 1024);
        
        cout << "\n=== Testing with matrix " << n << "x" << n 
             << " (~" << memory_mb << " MB) ===" << endl;
        
        // Выделяем память
        double* A = new double[matrix_size];
        double* x = new double[n];
        double* y_parallel = new double[n];
        double* y_serial = new double[n];
        
        if (!A || !x || !y_parallel || !y_serial) {
            cerr << "Error: Memory allocation failed for size " << n << endl;
            delete[] A;
            delete[] x;
            delete[] y_parallel;
            delete[] y_serial;
            continue;
        }
        
        // Получаем последовательное время выполнения
        cout << "Measuring sequential time..." << endl;
        
        // Инициализация для последовательной версии
        init_matrix_omp(A, n, 1);
        init_vector_omp(x, n, 1);
        
        double sequential_time = 0.0;
        
        // Прогрев для последовательной версии
        for (int iter = 0; iter < warmup_iterations; iter++) {
            matrix_vector_mult_serial(A, x, y_serial, n);
        }
        
        // Измерение последовательной версии
        for (int iter = 0; iter < measurement_iterations; iter++) {
            double start = omp_get_wtime();
            matrix_vector_mult_serial(A, x, y_serial, n);
            double end = omp_get_wtime();
            sequential_time += (end - start);
        }
        sequential_time /= measurement_iterations;
        
        cout << "Sequential time: " << sequential_time << " seconds" << endl;
        
        // Тестирование с разным количеством потоков
        for (int t = 0; t < num_threads_list; t++) {
            int threads = threads_list[t];
            
            cout << "Testing with " << threads << " threads..." << flush;
            
            // ВАЖНО: Инициализация с текущим количеством потоков для правильного NUMA-размещения
            init_matrix_omp(A, n, threads);
            init_vector_omp(x, n, threads);
            
            double total_time = 0.0;
            
            // Прогрев кэша
            for (int iter = 0; iter < warmup_iterations; iter++) {
                matrix_vector_mult_omp(A, x, y_parallel, n, threads);
            }
            
            // Измерение времени
            for (int iter = 0; iter < measurement_iterations; iter++) {
                double start = omp_get_wtime();
                matrix_vector_mult_omp(A, x, y_parallel, n, threads);
                double end = omp_get_wtime();
                total_time += (end - start);
            }
            
            double avg_time = total_time / measurement_iterations;
            double speedup = sequential_time / avg_time;
            
            cout << " done. Time: " << avg_time << " s, Speedup: " << speedup << endl;
            
            // Верификация для первого размера матрицы и первого теста
            if (s == 0 && t == 0) {
                cout << "Verifying results..." << endl;
                verify_results(y_parallel, y_serial, n);
            }
            
            data_file << n << " " << threads << " "
                     << avg_time << " " << speedup << "\n";
        }
        
        // Очистка памяти
        delete[] A;
        delete[] x;
        delete[] y_parallel;
        delete[] y_serial;
    }
    
    data_file.close();
    
    cout << "\n=== Benchmark completed ===" << endl;
    cout << "Results saved to benchmark_data.txt" << endl;
    
    return 0;
}