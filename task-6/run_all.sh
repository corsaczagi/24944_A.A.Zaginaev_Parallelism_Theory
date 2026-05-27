#!/bin/bash

set -e

HOST=./build/jacobi_host
MULTI=./build/jacobi_multi
GPU=./build/jacobi_gpu

echo ""
echo "=================================================="
echo "SMALL GRID CHECK"
echo "=================================================="
echo ""

for s in 10 13
do
    echo "[$s] ${s}x${s} grid"

    ${GPU} \
        --size $s \
        --eps 1e-6 \
        --max-iters 100000

    echo ""
done

echo ""
echo "=================================================="
echo "PERFORMANCE TESTS"
echo "=================================================="
echo ""

for s in 128 256 512 1024
do
    echo ""
    echo "----------------------------------------------"
    echo "GRID SIZE: ${s}x${s}"
    echo "----------------------------------------------"
    echo ""

    echo "[HOST]"

    ${HOST} \
        --size $s \
        --eps 1e-6 \
        --max-iters 1000000

    echo ""

    echo "[MULTICORE]"

    ${MULTI} \
        --size $s \
        --eps 1e-6 \
        --max-iters 1000000

    echo ""

    echo "[GPU]"

    ${GPU} \
        --size $s \
        --eps 1e-6 \
        --max-iters 1000000

    echo ""
done