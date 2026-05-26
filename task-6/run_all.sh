#!/bin/bash

# =========================================================
# TASK 6 TEST SCRIPT
# Heat equation + OpenACC
# =========================================================

set -e

SOURCE="heat.cpp"
EXEC="heat"

echo "=================================================="
echo "COMPILATION"
echo "=================================================="

# ---------------------------------------------------------
# GPU VERSION
# ---------------------------------------------------------

echo
echo "[1] GPU build"

nvc++ \
    -O2 \
    -acc=gpu \
    -Minfo=all \
    "$SOURCE" \
    -o "${EXEC}_gpu"

echo "GPU build completed"


# ---------------------------------------------------------
# CPU HOST VERSION
# ---------------------------------------------------------

echo
echo "[2] HOST build"

nvc++ \
    -O2 \
    -acc=host \
    -Minfo=all \
    "$SOURCE" \
    -o "${EXEC}_host"

echo "HOST build completed"


# ---------------------------------------------------------
# MULTICORE VERSION
# ---------------------------------------------------------

echo
echo "[3] MULTICORE build"

nvc++ \
    -O2 \
    -acc=multicore \
    -Minfo=all \
    "$SOURCE" \
    -o "${EXEC}_multi"

echo "MULTICORE build completed"


echo
echo "=================================================="
echo "SMALL GRID CHECK"
echo "=================================================="

echo
echo "[4] 10x10 grid"

./${EXEC}_host \
    --size 10 \
    --eps 1e-6 \
    --max-iters 1000 \
    --print


echo
echo "[5] 13x13 grid"

./${EXEC}_host \
    --size 13 \
    --eps 1e-6 \
    --max-iters 1000 \
    --print


echo
echo "=================================================="
echo "PERFORMANCE TESTS"
echo "=================================================="

sizes=(128 256 512 1024)

for s in "${sizes[@]}"
do
    echo
    echo "----------------------------------------------"
    echo "GRID SIZE: ${s}x${s}"
    echo "----------------------------------------------"

    echo
    echo "[HOST]"
    ./${EXEC}_host \
        --size $s \
        --eps 1e-6 \
        --max-iters 100000

    echo
    echo "[MULTICORE]"
    ./${EXEC}_multi \
        --size $s \
        --eps 1e-6 \
        --max-iters 100000

    echo
    echo "[GPU]"
    ./${EXEC}_gpu \
        --size $s \
        --eps 1e-6 \
        --max-iters 100000
done


echo
echo "=================================================="
echo "NSIGHT SYSTEMS PROFILING"
echo "=================================================="

echo
echo "[6] Creating Nsight profile"

nsys profile \
    --stats=true \
    -o heat_profile \
    ./${EXEC}_gpu \
    --size 512 \
    --max-iters 50

echo
echo "Nsight profile saved:"
echo "heat_profile.qdrep"
echo "heat_profile.sqlite"


echo
echo "=================================================="
echo "DONE"
echo "=================================================="