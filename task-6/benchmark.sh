#!/bin/bash

# Цветной вывод
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Heat Equation Solver - Test Suite${NC}"
echo -e "${BLUE}========================================${NC}"

# Создаем директории для сборки
mkdir -p build_gpu build_cpu build_multicore

# Проверка наличия компилятора
if ! command -v pgc++ &> /dev/null; then
    echo -e "${RED}Error: pgc++ not found. Please load NVIDIA HPC SDK module.${NC}"
    exit 1
fi

# Проверка наличия Boost
if ! find /usr -name "libboost_program_options*" 2>/dev/null | grep -q .; then
    echo -e "${YELLOW}Warning: Boost program_options not found. Attempting to continue...${NC}"
fi

# Функция для сборки
build() {
    local mode=$1
    local build_dir="build_${mode}"
    
    echo -e "\n${YELLOW}Building for ${mode}...${NC}"
    cd $build_dir
    
    cmake .. -DOPENACC_MODE=${mode} > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo -e "${RED}CMake configuration failed for ${mode}${NC}"
        cd ..
        return 1
    fi
    
    make 2>&1 | grep -E "(acc|Generating|Error|warning)" | head -20
    if [ $? -ne 0 ] && [ ! -f heat_solver ]; then
        echo -e "${RED}Build failed for ${mode}${NC}"
        cd ..
        return 1
    fi
    
    cd ..
    echo -e "${GREEN}✓ Build successful for ${mode}${NC}"
    return 0
}

# Функция для запуска тестов
run_tests() {
    local mode=$1
    local build_dir="build_${mode}"
    local executable="${build_dir}/heat_solver"
    
    if [ ! -f "$executable" ]; then
        echo -e "${RED}Executable not found for ${mode}${NC}"
        return 1
    fi
    
    echo -e "\n${BLUE}=== Testing ${mode} mode ===${NC}"
    
    # Тест 1: Проверка сетки 10x10 (вывод сетки)
    echo -e "\n${YELLOW}Test 1: Grid 10x10 (check boundary conditions)${NC}"
    $executable --size=10 --eps=1e-6 --iter=100 | tee /tmp/output_10x10.txt
    
    # Сохраняем скриншот вывода
    if [ -f /tmp/output_10x10.txt ]; then
        echo -e "${GREEN}✓ Output saved to /tmp/output_10x10.txt${NC}"
    fi
    
    # Тест 2: Проверка сетки 13x13
    echo -e "\n${YELLOW}Test 2: Grid 13x13 (check final grid)${NC}"
    $executable --size=13 --eps=1e-6 --iter=100
    
    # Тест 3: Разные размеры сеток для производительности
    echo -e "\n${YELLOW}Test 3: Performance tests${NC}"
    
    for size in 128 256 512 1024; do
        echo -e "\n${BLUE}Grid size: ${size}x${size}${NC}"
        $executable --size=$size --eps=1e-6 --iter=1000 2>&1 | grep -E "(Grid:|Iterations:|Time:|Performance:|RESULT:)" | head -10
    done
    
    # Тест 4: Проверка точности
    echo -e "\n${YELLOW}Test 4: Convergence test (eps=1e-6, max_iter=50000)${NC}"
    $executable --size=256 --eps=1e-6 --iter=50000 2>&1 | grep -E "(Iterations:|Final error:)"
    
    # Тест 5: Профилировочный запуск (30-100 итераций)
    echo -e "\n${YELLOW}Test 5: Profiling run (50 iterations)${NC}"
    $executable --size=512 --eps=1e-3 --iter=50 2>&1 | grep -E "(Iteration|RESULT:|Time:)"
}

# Функция для профилирования с Nsight Systems
run_profiling() {
    local mode=$1
    local build_dir="build_${mode}"
    local executable="${build_dir}/heat_solver"
    
    if [ ! -f "$executable" ]; then
        echo -e "${RED}Executable not found for ${mode}${NC}"
        return 1
    fi
    
    echo -e "\n${BLUE}=== Profiling ${mode} mode with Nsight Systems ===${NC}"
    
    # Проверяем доступность nsys
    if ! command -v nsys &> /dev/null; then
        echo -e "${RED}Nsight Systems not found. Skipping profiling.${NC}"
        return 1
    fi
    
    local profile_file="profile_${mode}"
    nsys profile -o ${profile_file} -f true \
        --stats=true \
        $executable --size=512 --eps=1e-3 --iter=50 > /dev/null 2>&1
    
    if [ -f "${profile_file}.qdrep" ] || [ -f "${profile_file}.nsys-rep" ]; then
        echo -e "${GREEN}✓ Profile saved to ${profile_file}.nsys-rep${NC}"
        echo -e "${YELLOW}View with: nsys-ui ${profile_file}.nsys-rep${NC}"
    else
        echo -e "${RED}Profiling failed${NC}"
    fi
}

# Функция для сравнения производительности
compare_performance() {
    echo -e "\n${BLUE}========================================${NC}"
    echo -e "${BLUE}Performance Comparison${NC}"
    echo -e "${BLUE}========================================${NC}"
    
    local results_file="/tmp/perf_results.txt"
    echo "Mode,Size,Iterations,Time,Error" > $results_file
    
    for mode in cpu multicore gpu; do
        build_dir="build_${mode}"
        executable="${build_dir}/heat_solver"
        
        if [ -f "$executable" ]; then
            echo -e "\n${YELLOW}Testing ${mode} mode...${NC}"
            for size in 256 512 1024; do
                output=$($executable --size=$size --eps=1e-6 --iter=1000 2>&1)
                iter=$(echo "$output" | grep "Iterations:" | awk '{print $2}')
                time=$(echo "$output" | grep "Time:" | awk '{print $2}')
                error=$(echo "$output" | grep "Final error:" | awk '{print $3}')
                echo "${mode},${size},${iter},${time},${error}" >> $results_file
            done
        else
            echo -e "${RED}${mode} executable not found${NC}"
        fi
    done
    
    # Выводим таблицу результатов
    echo -e "\n${GREEN}Results:${NC}"
    column -t -s',' $results_file
}

# Функция для проверки OpenACC информационных сообщений
check_openacc_info() {
    local mode=$1
    local build_dir="build_${mode}"
    
    echo -e "\n${BLUE}=== OpenACC Info for ${mode} mode ===${NC}"
    
    # Пересобираем для получения сообщений -Minfo
    cd $build_dir
    make clean > /dev/null 2>&1
    make 2>&1 | grep -E "(acc|Generating|loop|vector|gang|worker)" | head -30
    cd ..
}

# Основная логика
case "${1}" in
    build)
        build "gpu"
        build "cpu"
        build "multicore"
        ;;
    test)
        for mode in cpu multicore gpu; do
            if [ -f "build_${mode}/heat_solver" ]; then
                run_tests $mode
            else
                echo -e "${RED}Build ${mode} first: ./run_tests.sh build${NC}"
            fi
        done
        ;;
    profile)
        for mode in cpu multicore gpu; do
            if [ -f "build_${mode}/heat_solver" ]; then
                run_profiling $mode
            fi
        done
        ;;
    compare)
        compare_performance
        ;;
    info)
        for mode in cpu multicore gpu; do
            check_openacc_info $mode
        done
        ;;
    all)
        echo -e "${BLUE}Running complete test suite...${NC}"
        $0 build
        $0 test
        $0 compare
        $0 info
        echo -e "\n${GREEN}All tests completed!${NC}"
        ;;
    clean)
        echo -e "${YELLOW}Cleaning build directories...${NC}"
        rm -rf build_* profile_* /tmp/output_*.txt
        echo -e "${GREEN}Clean complete${NC}"
        ;;
    *)
        echo "Usage: $0 {build|test|profile|compare|info|all|clean}"
        echo ""
        echo "Commands:"
        echo "  build    - Build for GPU, CPU, and multicore"
        echo "  test     - Run all tests for all builds"
        echo "  profile  - Profile with Nsight Systems"
        echo "  compare  - Compare performance between modes"
        echo "  info     - Show OpenACC compiler info"
        echo "  all      - Run everything (build, test, compare, info)"
        echo "  clean    - Remove build files"
        echo ""
        echo "Example:"
        echo "  ./run_tests.sh all"
        echo "  ./run_tests.sh compare"
        exit 1
        ;;
esac

echo -e "\n${GREEN}Done!${NC}"