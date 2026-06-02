#!/bin/bash

echo "=== Heat Equation Solver Benchmark ==="
echo ""

SIZES=(128 256 512 1024)
MAX_ITER=1000000
EPS=1e-6

for SIZE in "${SIZES[@]}"; do
    echo "----------------------------------------"
    echo "Grid size: ${SIZE}x${SIZE}"
    echo ""
    
    # CPU (один поток)
    echo "CPU (single core):"
    ./heat_solver_cpu --size $SIZE --eps $EPS --iter $MAX_ITER | grep -E "(Iterations:|Time:)"
    echo ""
    
    # CPU (многоядерный)
    echo "CPU (multicore):"
    ./heat_solver_multicore --size $SIZE --eps $EPS --iter $MAX_ITER | grep -E "(Iterations:|Time:)"
    echo ""
    
    # GPU
    echo "GPU:"
    ./heat_solver_gpu --size $SIZE --eps $EPS --iter $MAX_ITER | grep -E "(Iterations:|Time:)"
    echo ""
done

# Профилирование для размера 512 (30-100 итераций)
echo "=== Profiling with Nsight Systems (size=512, iter=50) ==="
nsys profile --trace=cuda,openacc --stats=true -o heat_profile_512 ./heat_solver_gpu --size 512 --eps 1e-6 --iter 50