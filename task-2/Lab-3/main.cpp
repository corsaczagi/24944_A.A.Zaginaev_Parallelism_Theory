#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <fstream>  
#include <omp.h>

#ifndef N_SIZE
#define N_SIZE 2000
#endif

#ifndef NUM_THREADS
#define NUM_THREADS 8
#endif

#ifndef MAX_ITER
#define MAX_ITER 10000
#endif

#ifndef EPS
#define EPS 1e-6
#endif

double get_wall_time() {
    struct timespec time_spec;
    timespec_get(&time_spec, TIME_UTC);
    return static_cast<double>(time_spec.tv_sec) + static_cast<double>(time_spec.tv_nsec) * 1e-9;
}

void solve_serial(const std::vector<double>& matrix, const std::vector<double>& rhs, 
                  std::vector<double>& sol, int dim, double rhs_norm) {
    const double param_tau = 0.8 / (dim + 1);
    int step = 0;
    double current_residual = 1.0;
    
    while (step < MAX_ITER && (current_residual / rhs_norm) > EPS) {
        current_residual = 0.0;
        for (int i = 0; i < dim; ++i) {
            double mx = 0.0;
            for (int j = 0; j < dim; ++j) {
                mx += matrix[i * dim + j] * sol[j];
            }
            double diff = rhs[i] - mx;
            sol[i] += param_tau * diff;
            current_residual += diff * diff;
        }
        current_residual = std::sqrt(current_residual);
        step++;
    }
    std::cout << "[Serial] " << step << " iterations, rel_norm = " 
              << std::scientific << (current_residual / rhs_norm) << "\n";
}

double test_serial(const std::vector<double>& matrix, const std::vector<double>& rhs, 
                   int dim, double rhs_norm) {
    std::vector<double> sol(dim, 0.0);
    
    double start_time = get_wall_time();
    solve_serial(matrix, rhs, sol, dim, rhs_norm);
    double end_time = get_wall_time() - start_time;
    
    double max_err = 0.0;
    for (int i = 0; i < dim; ++i) {
        double err = std::fabs(sol[i] - 1.0);
        if (err > max_err) max_err = err;
    }
    
    std::cout << "  Time: " << std::fixed << std::setprecision(6) << end_time << " s\n";
    std::cout << "  Max error: " << std::scientific << max_err << "\n\n";
    
    return end_time;
}

void solve_omp_split(const std::vector<double>& matrix, const std::vector<double>& rhs, 
                     std::vector<double>& sol, int dim, double rhs_norm) {
    const double param_tau = 0.8 / (dim + 1);
    int step = 0;
    double current_residual = 1.0;
    
    while (step < MAX_ITER && (current_residual / rhs_norm) > EPS) {
        #pragma omp parallel for
        for (int i = 0; i < dim; ++i) {
            double mx = 0.0;
            for (int j = 0; j < dim; ++j) {
                mx += matrix[i * dim + j] * sol[j];
            }
            double diff = rhs[i] - mx;
            sol[i] += param_tau * diff;
        }
        
        double local_norm = 0.0;
        #pragma omp parallel for reduction(+:local_norm)
        for (int i = 0; i < dim; ++i) {
            double mx = 0.0;
            for (int j = 0; j < dim; ++j) {
                mx += matrix[i * dim + j] * sol[j];
            }
            double diff = rhs[i] - mx;
            local_norm += diff * diff;
        }
        
        current_residual = std::sqrt(local_norm);
        step++;
    }
    std::cout << "[OMP Split] " << step << " iterations, rel_norm = " 
              << std::scientific << (current_residual / rhs_norm) << "\n";
}

double test_omp_split(const std::vector<double>& matrix, const std::vector<double>& rhs, 
                      int dim, double rhs_norm) {
    std::vector<double> sol(dim, 0.0);
    
    double start_time = get_wall_time();
    solve_omp_split(matrix, rhs, sol, dim, rhs_norm);
    double end_time = get_wall_time() - start_time;
    
    double max_err = 0.0;
    for (int i = 0; i < dim; ++i) {
        double err = std::fabs(sol[i] - 1.0);
        if (err > max_err) max_err = err;
    }
    
    std::cout << "  Time: " << std::fixed << std::setprecision(6) << end_time << " s\n";
    std::cout << "  Max error: " << std::scientific << max_err << "\n\n";
    
    return end_time;
}

void solve_omp_single_region(const std::vector<double>& matrix, const std::vector<double>& rhs, 
                             std::vector<double>& sol, int dim, double rhs_norm) {
    const double param_tau = 0.8 / (dim + 1);
    int step = 0;
    double current_residual = 1.0;
    
    while (step < MAX_ITER && (current_residual / rhs_norm) > EPS) {
        current_residual = 0.0;
        
        #pragma omp parallel
        {
            int t_count = omp_get_num_threads();
            int t_id = omp_get_thread_num();
            int chunk_size = dim / t_count;
            int start_idx = t_id * chunk_size;
            int end_idx = (t_id == t_count - 1) ? (dim - 1) : (start_idx + chunk_size - 1);
            
            double local_norm = 0.0;
            
            for (int i = start_idx; i <= end_idx; ++i) {
                double mx = 0.0;
                for (int j = 0; j < dim; ++j) {
                    mx += matrix[i * dim + j] * sol[j];
                }
                double diff = rhs[i] - mx;
                sol[i] += param_tau * diff;
                local_norm += diff * diff;
            }
            
            #pragma omp atomic
            current_residual += local_norm;
        }
        
        current_residual = std::sqrt(current_residual);
        step++;
    }
    std::cout << "[OMP Single] " << step << " iterations, rel_norm = " 
              << std::scientific << (current_residual / rhs_norm) << "\n";
}

double test_omp_single_region(const std::vector<double>& matrix, const std::vector<double>& rhs, 
                              int dim, double rhs_norm) {
    std::vector<double> sol(dim, 0.0);
    
    double start_time = get_wall_time();
    solve_omp_single_region(matrix, rhs, sol, dim, rhs_norm);
    double end_time = get_wall_time() - start_time;
    
    double max_err = 0.0;
    for (int i = 0; i < dim; ++i) {
        double err = std::fabs(sol[i] - 1.0);
        if (err > max_err) max_err = err;
    }
    
    std::cout << "  Time: " << std::fixed << std::setprecision(6) << end_time << " s\n";
    std::cout << "  Max error: " << std::scientific << max_err << "\n\n";
    
    return end_time;
}

void solve_omp_schedule(const std::vector<double>& matrix, const std::vector<double>& rhs, 
                        std::vector<double>& sol, int dim, const char* sched_type, 
                        double rhs_norm, int num_threads) {
    const double param_tau = 0.8 / (dim + 1);
    int step = 0;
    double current_residual = 1.0;
    
    omp_set_num_threads(num_threads);
    
    std::string sched_str(sched_type);
    omp_sched_t sched_kind;
    int chunk = 0;
    
    if (sched_str.find("static") == 0) {
        sched_kind = omp_sched_static;
        if (sched_str.find(",") != std::string::npos) {
            chunk = std::stoi(sched_str.substr(sched_str.find(",") + 1));
        }
    } else if (sched_str.find("dynamic") == 0) {
        sched_kind = omp_sched_dynamic;
        if (sched_str.find(",") != std::string::npos) {
            chunk = std::stoi(sched_str.substr(sched_str.find(",") + 1));
        } else {
            chunk = 64;
        }
    } else if (sched_str.find("guided") == 0) {
        sched_kind = omp_sched_guided;
        if (sched_str.find(",") != std::string::npos) {
            chunk = std::stoi(sched_str.substr(sched_str.find(",") + 1));
        }
    } else {
        sched_kind = omp_sched_auto;
    }
    
    omp_set_schedule(sched_kind, chunk);
    
    while (step < MAX_ITER && (current_residual / rhs_norm) > EPS) {
        #pragma omp parallel for schedule(runtime)
        for (int i = 0; i < dim; ++i) {
            double mx = 0.0;
            for (int j = 0; j < dim; ++j) {
                mx += matrix[i * dim + j] * sol[j];
            }
            double diff = rhs[i] - mx;
            sol[i] += param_tau * diff;
        }
        
        double local_norm = 0.0;
        #pragma omp parallel for reduction(+:local_norm)
        for (int i = 0; i < dim; ++i) {
            double mx = 0.0;
            for (int j = 0; j < dim; ++j) {
                mx += matrix[i * dim + j] * sol[j];
            }
            double diff = rhs[i] - mx;
            local_norm += diff * diff;
        }
        
        current_residual = std::sqrt(local_norm);
        step++;
    }
    
    std::cout << "  " << std::setw(15) << std::left << sched_type 
              << " | iter: " << std::setw(5) << step 
              << " | rel_norm: " << std::scientific << (current_residual / rhs_norm) << "\n";
}

double test_omp_schedule(const std::vector<double>& matrix, const std::vector<double>& rhs, 
                         int dim, const char* sched_type, double rhs_norm, int num_threads) {
    std::vector<double> sol(dim, 0.0);
    
    double start_time = get_wall_time();
    solve_omp_schedule(matrix, rhs, sol, dim, sched_type, rhs_norm, num_threads);
    double end_time = get_wall_time() - start_time;
    
    double max_err = 0.0;
    for (int i = 0; i < dim; ++i) {
        double err = std::fabs(sol[i] - 1.0);
        if (err > max_err) max_err = err;
    }
    
    std::cout << "    Time: " << std::fixed << std::setprecision(4) << end_time << " s"
              << ", error: " << std::scientific << max_err << "\n\n";
    
    return end_time;
}

int main(int argc, char** argv) {
    int dim = N_SIZE;
    if (argc > 1) dim = std::atoi(argv[1]);
    
    std::ofstream outfile("results.txt");
    outfile << "# Threads Serial_Time Split_Time Single_Time Speedup_Split Speedup_Single\n";
    
    int threads_array[] = {1, 2, 4, 7, 8, 16, 20, 40};
    int num_threads_configs = sizeof(threads_array) / sizeof(threads_array[0]);
    
    std::cout << "SLAU Solver (Richardson iteration)\n";
    std::cout << "N = " << dim << ", tau = " << 0.8/(dim+1) << "\n";
    
    std::vector<double> matrix(dim * dim);
    std::vector<double> rhs(dim);
    
    #pragma omp parallel for
    for (int i = 0; i < dim; ++i) {
        for (int j = 0; j < dim; ++j) {
            matrix[i * dim + j] = (i == j) ? 2.0 : 1.0;
        }
        rhs[i] = dim + 1.0;
    }
    
    double rhs_norm = 0.0;
    for (double v : rhs) rhs_norm += v * v;
    rhs_norm = std::sqrt(rhs_norm);
    
    double serial_time = test_serial(matrix, rhs, dim, rhs_norm);
    
    for (int idx = 0; idx < num_threads_configs; ++idx) {
        int n_threads = threads_array[idx];
        omp_set_num_threads(n_threads);
        
        std::cout << "--- Testing with " << n_threads << " threads ---\n";
        
        double split_time = test_omp_split(matrix, rhs, dim, rhs_norm);
        double single_time = test_omp_single_region(matrix, rhs, dim, rhs_norm);
        
        double speedup_split = serial_time / split_time;
        double speedup_single = serial_time / single_time;
        
        outfile << n_threads << " " << serial_time << " " << split_time << " " 
                << single_time << " " << speedup_split << " " << speedup_single << "\n";
    }
    
    outfile.close();

    std::cout << "\n=== Schedule study (threads=8) ===\n\n";
    
    const char* schedules[] = {
        "static", "static,64", "static,128",
        "dynamic", "dynamic,64", "dynamic,128",
        "guided", "guided,64",
        "auto"
    };
    
    int num_schedules = sizeof(schedules) / sizeof(schedules[0]);
    
    std::cout << std::left << std::setw(20) << "Schedule" 
              << " | Time (s) | Rel. norm\n";
    std::cout << std::string(55, '-') << "\n";
    
    for (int i = 0; i < num_schedules; ++i) {
        test_omp_schedule(matrix, rhs, dim, schedules[i], rhs_norm, 8); 
    }
    std::cout << "\nResults saved to results.txt\n";
    
    return 0;
}
