#include <iostream>
#include <omp.h>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <cstdlib>

using namespace std;

void init_matrix_omp(double* A, int n) {
    #pragma omp parallel
    {
        int nthreads = omp_get_num_threads();
        int threadid = omp_get_thread_num();
        int rows_per_thread = n / nthreads;
        int start = threadid * rows_per_thread;
        int end = (threadid == nthreads - 1) ? n : (threadid + 1) * rows_per_thread;
        
        for (int i = start; i < end; i++) {
            for (int j = 0; j < n; j++) {
                A[i * n + j] = sin(i * 0.001) * cos(j * 0.001);
            }
        }
    }
}

void init_vector_omp(double* x, int n) {
    #pragma omp parallel
    {
        int nthreads = omp_get_num_threads();
        int threadid = omp_get_thread_num();
        int elems_per_thread = n / nthreads;
        int start = threadid * elems_per_thread;
        int end = (threadid == nthreads - 1) ? n : (threadid + 1) * elems_per_thread;
        
        for (int i = start; i < end; i++) {
            x[i] = exp(i * 0.0001);
        }
    }
}

void matrix_vector_mult_omp(const double* A, const double* x, double* y, int n) {
    #pragma omp parallel
    {
        int nthreads = omp_get_num_threads();
        int threadid = omp_get_thread_num();
        int rows_per_thread = n / nthreads;
        int start = threadid * rows_per_thread;
        int end = (threadid == nthreads - 1) ? n : (threadid + 1) * rows_per_thread;
        
        for (int i = start; i < end; i++) {
            double sum = 0.0;
            for (int j = 0; j < n; j++) {
                sum += A[i * n + j] * x[j];
            }
            y[i] = sum;
        }
    }
}

void print_system_info() {
    system("lscpu | grep -E 'Model name|CPU\\(s\\)|Thread\\(s\\) per core|Core\\(s\\) per socket|Socket\\(s\\)|L3 cache'");
    system("cat /sys/devices/virtual/dmi/id/product_name 2>/dev/null || echo 'Недоступно'");
    system("numactl --hardware 2>/dev/null | grep -A 10 'available' || echo 'numactl не установлен'");
    system("free -h | grep Mem");
    system("cat /etc/os-release | grep -E 'PRETTY_NAME|VERSION' | head -2");
}

int main() {
    print_system_info();
    
    int sizes[] = {20000, 40000};
    int threads_list[] = {1, 2, 4, 7, 8, 16, 20, 40};
    int num_sizes = 2;
    int num_threads_list = 8;
    
    ofstream data_file("benchmark_data.txt");
    if (!data_file.is_open()) {
        return 1;
    }
    
    data_file << "# размер_матрицы потоки время(с) ускорение\n";
    
    for (int s = 0; s < num_sizes; s++) {
        int n = sizes[s];
        
        double* A = new double[(long long)n * n];
        double* x = new double[n];
        double* y = new double[n];
        
        omp_set_num_threads(4);
        init_matrix_omp(A, n);
        init_vector_omp(x, n);
        
        double base_time = 0.0;
        
        for (int t = 0; t < num_threads_list; t++) {
            int threads = threads_list[t];
            
            double total_time = 0.0;
            const int iterations = 3;
            
            for (int iter = 0; iter < iterations; iter++) {
                omp_set_num_threads(threads);
                
                double start = omp_get_wtime();
                matrix_vector_mult_omp(A, x, y, n);
                double end = omp_get_wtime();
                
                total_time += (end - start);
            }
            
            double avg_time = total_time / iterations;
            
            if (threads == 1) {
                base_time = avg_time;
            }
            
            double speedup = base_time / avg_time;
            
            data_file << n << " " << threads << " "
                     << avg_time << " " << speedup << "\n";
        }
        
        delete[] A;
        delete[] x;
        delete[] y;
    }
    
    data_file.close();
    
    return 0;
}