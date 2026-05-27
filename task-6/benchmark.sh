#!/bin/bash

echo "Heat Equation Solver - Performance Benchmark"
echo "============================================"
echo ""

# Размеры сеток
SIZES=(128 256 512 1024)
PRECISION=1e-6

# Файл для результатов
RESULT_FILE="results.csv"
echo "Size,Device,Iterations,Time(s),MUpdates/s" > $RESULT_FILE

for size in "${SIZES[@]}"; do
    echo "Testing grid size: ${size}x${size}"
    
    # CPU
    echo "  CPU..."
    OUTPUT=$(./heat_solver --size $size --precision $PRECISION --cpu 2>/dev/null)
    ITER=$(echo "$OUTPUT" | grep "Iterations:" | awk '{print $2}')
    TIME=$(echo "$OUTPUT" | grep "Time:" | awk '{print $2}')
    UPD=$(echo "$OUTPUT" | grep "M update/s" | awk '{print $3}')
    echo "$size,CPU,$ITER,$TIME,$UPD" >> $RESULT_FILE
    
    # GPU
    echo "  GPU..."
    OUTPUT=$(./heat_solver --size $size --precision $PRECISION 2>/dev/null)
    ITER=$(echo "$OUTPUT" | grep "Iterations:" | awk '{print $2}')
    TIME=$(echo "$OUTPUT" | grep "Time:" | awk '{print $2}')
    UPD=$(echo "$OUTPUT" | grep "M update/s" | awk '{print $3}')
    echo "$size,GPU,$ITER,$TIME,$UPD" >> $RESULT_FILE
    
    echo ""
done

echo "Benchmark complete. Results saved to $RESULT_FILE"
echo ""
echo "Summary:"
echo "========"
cat $RESULT_FILE