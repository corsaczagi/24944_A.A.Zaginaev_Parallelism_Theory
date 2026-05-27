#!/bin/bash

SIZES=(128 256 512 1024)
PRECISION=1e-6
ITERATIONS=1000

echo "Size,Device,Time(s),Iterations,Error" > results.csv

for size in "${SIZES[@]}"; do
    echo "Testing size: ${size}x${size}"
    
    # CPU
    echo "  CPU..."
    output=$(./heat_solver --size $size --precision $PRECISION --iterations $ITERATIONS 2>/dev/null)
    time=$(echo "$output" | grep "Time:" | awk '{print $2}')
    iter=$(echo "$output" | grep "Iterations:" | awk '{print $2}')
    error=$(echo "$output" | grep "Final error:" | awk '{print $3}')
    echo "$size,CPU,$time,$iter,$error" >> results.csv
    
    # GPU
    echo "  GPU..."
    output=$(ACC_DEVICE_TYPE=nvidia ./heat_solver --size $size --precision $PRECISION --iterations $ITERATIONS 2>/dev/null)
    time=$(echo "$output" | grep "Time:" | awk '{print $2}')
    iter=$(echo "$output" | grep "Iterations:" | awk '{print $2}')
    error=$(echo "$output" | grep "Final error:" | awk '{print $3}')
    echo "$size,GPU,$time,$iter,$error" >> results.csv
    
    # Multicore
    echo "  Multicore..."
    output=$(ACC_NUM_CORES=8 ./heat_solver --size $size --precision $PRECISION --iterations $ITERATIONS 2>/dev/null)
    time=$(echo "$output" | grep "Time:" | awk '{print $2}')
    iter=$(echo "$output" | grep "Iterations:" | awk '{print $2}')
    error=$(echo "$output" | grep "Final error:" | awk '{print $3}')
    echo "$size,Multicore,$time,$iter,$error" >> results.csv
done

echo "Benchmark complete. Results in results.csv"