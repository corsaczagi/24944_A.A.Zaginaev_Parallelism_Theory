// matrix_vector_mult_simple_info.cpp
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <cstdlib>

using namespace std;
using namespace chrono;

// ---------- Таймер ----------
class Timer {
    high_resolution_clock::time_point start;
public:
    Timer() { reset(); }
    void reset() { start = high_resolution_clock::now(); }
    double elapsed() { 
        return duration_cast<milliseconds>(high_resolution_clock::now() - start).count() / 1000.0;
    }
};

// ---------- Функции для заполнения данных (как в оригинале) ----------
void fill_matrix_row(double* row, int n, int row_idx) {
    for (int j = 0; j < n; j++) {
        row[j] = sin(row_idx * 0.001) * cos(j * 0.001);
    }
}

void fill_vector(double* vec, int n) {
    for (int i = 0; i < n; i++) {
        vec[i] = exp(i * 0.0001);
    }
}

// ---------- Умножение части матрицы на вектор ----------
void mult_part(const double* A, const double* x, double* y, int n, int start, int end) {
    for (int i = start; i < end; i++) {
        double sum = 0;
        const double* row = A + (long long)i * n;
        for (int j = 0; j < n; j++) {
            sum += row[j] * x[j];
        }
        y[i] = sum;
    }
}

// ---------- Информация о вычислительном узле ----------
void print_system_info() {
    cout << "\nИНФОРМАЦИЯ О ВЫЧИСЛИТЕЛЬНОМ УЗЛЕ\n";
    system("lscpu | grep -E 'Model name|CPU\\(s\\)|Thread\\(s\\) per core|Core\\(s\\) per socket|Socket\\(s\\)|L3 cache'");
    cout << "\n--- Модель сервера ---\n";
    system("cat /sys/devices/virtual/dmi/id/product_name 2>/dev/null || echo 'Недоступно'");
    cout << "\n--- NUMA ноды ---\n";
    system("numactl --hardware 2>/dev/null | grep -A 10 'available' || echo 'numactl не установлен'");
    cout << "\n--- Оперативная память ---\n";
    system("free -h | grep Mem");
    cout << "\n--- Операционная система ---\n";
    system("cat /etc/os-release | grep -E 'PRETTY_NAME|VERSION' | head -2");
}

// ---------- Главная функция ----------
int main(int argc, char* argv[]) {
    // Если аргументов нет – просто показываем информацию о системе
    if (argc == 1) {
        print_system_info();
        return 0;
    }
    
    // Режим сбора данных для графиков
    if (argc == 2 && string(argv[1]) == "--benchmark") {
        cout << "\n=== СБОР ДАННЫХ ДЛЯ ГРАФИКОВ ===\n";
        
        // Для контекста покажем информацию об узле
        print_system_info();
        
        // Параметры тестирования (как в исходном задании)
        vector<int> sizes = {20000, 40000};
        vector<int> threads_list = {1, 2, 4, 7, 8, 16, 20, 40};
        
        // Открываем файл для записи
        ofstream data_file("benchmark_data.txt");
        if (!data_file.is_open()) {
            cerr << "Ошибка: не удалось создать файл benchmark_data.txt\n";
            return 1;
        }
        
        data_file << "# Результаты тестирования (простая версия с информацией об узлах)\n";
        data_file << "# Формат: размер_матрицы количество_потоков время_выполнения ускорение\n\n";
        
        // Тестируем каждый размер матрицы
        for (int size : sizes) {
            cout << "\nРазмер матрицы: " << size << "x" << size << "\n";
            double mem_gb = (size * size * sizeof(double)) / (1024.0 * 1024.0 * 1024.0);
            cout << "Память под матрицу: " << fixed << setprecision(2) << mem_gb << " GB\n";
            
            // Выделяем память и заполняем данные ОДИН РАЗ для этого размера
            double* A = new double[(long long)size * size];
            double* x = new double[size];
            double* y = new double[size];   // для результата
            
            cout << "  Генерация матрицы... " << flush;
            for (int i = 0; i < size; i++) {
                fill_matrix_row(A + (long long)i * size, size, i);
            }
            cout << "готово\n";
            
            cout << "  Генерация вектора... " << flush;
            fill_vector(x, size);
            cout << "готово\n";
            
            double base_time = 0;   // время выполнения с 1 потоком
            
            // Перебираем разные количества потоков
            for (int threads : threads_list) {
                cout << "  Потоков " << setw(2) << threads << ": " << flush;
                
                double total_time = 0;
                const int iterations = 3;   // делаем 3 замера и усредняем
                
                for (int iter = 0; iter < iterations; iter++) {
                    Timer timer;
                    
                    // Запускаем потоки для умножения
                    vector<thread> workers;
                    int rows_per_thread = size / threads;
                    
                    for (int t = 0; t < threads; t++) {
                        int start = t * rows_per_thread;
                        int end = (t == threads-1) ? size : (t+1) * rows_per_thread;
                        workers.emplace_back(mult_part, A, x, y, size, start, end);
                    }
                    for (auto& w : workers) w.join();
                    
                    total_time += timer.elapsed();
                }
                
                double avg_time = total_time / iterations;
                
                if (threads == 1) base_time = avg_time;
                double speedup = base_time / avg_time;
                
                cout << fixed << setprecision(2) << avg_time << " с, "
                     << "ускорение " << setprecision(2) << speedup << endl;
                
                data_file << size << " " << threads << " "
                         << avg_time << " " << speedup << "\n";
            }
            
            // Освобождаем память после тестов для этого размера
            delete[] A;
            delete[] x;
            delete[] y;
        }
        
        data_file.close();
        cout << "\n✓ Данные сохранены в файл: benchmark_data.txt\n";
        cout << "  Теперь можно запустить Python-скрипт для построения графиков.\n";
        
        return 0;
    }
    
    // Если аргументы не соответствуют ни одному из режимов
    cout << "Использование:\n";
    cout << "  " << argv[0] << "                - информация о системе\n";
    cout << "  " << argv[0] << " --benchmark    - сбор данных для графиков\n";
    return 0;
}