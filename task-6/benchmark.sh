#!/bin/bash

EPS=1e-6
MAX_ITER=1000000

SIZES=(
128
256
512
1024
)

echo "==========================================="
echo " Heat Equation Benchmark"
echo "==========================================="

for SIZE in "${SIZES[@]}"
do

echo ""
echo "###########################################"
echo "GRID ${SIZE}x${SIZE}"
echo "###########################################"

echo ""
echo "HOST"
./heat_host \
    --size ${SIZE} \
    --eps ${EPS} \
    --iter ${MAX_ITER} \
| grep -E "Iterations|Final error|Time"

echo ""
echo "MULTICORE"
./heat_multicore \
    --size ${SIZE} \
    --eps ${EPS} \
    --iter ${MAX_ITER} \
| grep -E "Iterations|Final error|Time"

echo ""
echo "GPU"
./heat_gpu \
    --size ${SIZE} \
    --eps ${EPS} \
    --iter ${MAX_ITER} \
| grep -E "Iterations|Final error|Time"

done