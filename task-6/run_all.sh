#!/bin/bash

set -e

echo "=============================="
echo "BUILD HOST VERSION"
echo "=============================="

pgc++ jacobi.cpp 
-O3 
-Minfo=all 
-acc=host 
-o jacobi_host

echo ""
echo "=============================="
echo "BUILD MULTICORE VERSION"
echo "=============================="

pgc++ jacobi.cpp 
-O3 
-Minfo=all 
-acc=multicore 
-o jacobi_multi

echo ""
echo "=============================="
echo "BUILD GPU VERSION"
echo "=============================="

pgc++ jacobi.cpp 
-O3 
-Minfo=all 
-acc=gpu 
-gpu=managed 
-o jacobi_gpu

echo ""
echo "=============================="
echo "BUILD FINISHED"
echo "=============================="
