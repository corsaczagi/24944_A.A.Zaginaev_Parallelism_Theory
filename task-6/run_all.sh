#!/bin/bash

echo "=========================================="
echo "Задание 6: Решение уравнения теплопроводности"
echo "=========================================="
echo ""

# Компиляция
echo "=== Компиляция ==="
g++ -O2 jacobi.cpp -o jacobi_seq
g++ -O2 -fopenmp jacobi.cpp -o jacobi_omp
echo "  ✅ jacobi_seq"
echo "  ✅ jacobi_omp"
echo ""

# 1. Печать сетки 10x10 и 13x13
echo "=== 1. Печать сетки (для проверки) ==="
echo ""
echo "--- Сетка 10x10 ---"
./jacobi_seq -s 10 -p
echo ""
echo "--- Сетка 13x13 ---"
./jacobi_seq -s 13 -p
echo ""

# 2. Сохранение в файлы
mkdir -p results
./jacobi_seq -s 10 -p > results/grid_10x10.txt 2>&1
./jacobi_seq -s 13 -p > results/grid_13x13.txt 2>&1
echo "=== 2. Результаты сохранены ==="
echo "  ✅ results/grid_10x10.txt"
echo "  ✅ results/grid_13x13.txt"
echo ""

# 3. Тестирование производительности
echo "=== 3. Тестирование производительности ==="
echo ""

# Функция для запуска
run_test() {
    local cmd=$1
    local name=$2
    local size=$3
    
    echo "=========================================="
    echo "$name: $size x $size"
    echo "=========================================="
    $cmd -s $size -e 1e-6
    echo ""
}

# Запускаем тесты
for size in 128 256 512 1024; do
    run_test "./jacobi_seq" "SEQUENTIAL" $size
    run_test "./jacobi_omp" "OPENMP" $size
done

echo "=== 4. Готово ==="
echo "Результаты в папке results/"