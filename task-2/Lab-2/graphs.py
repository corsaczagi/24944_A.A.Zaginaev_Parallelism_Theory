#!/usr/bin/env python3
import matplotlib.pyplot as plt
import os

def main():
    possible_paths = [
        'integrate_benchmark.txt',
        'build/integrate_benchmark.txt',
        '../build/integrate_benchmark.txt'
    ]
    
    filename = None
    for path in possible_paths:
        if os.path.exists(path):
            filename = path
            break
    
    if filename is None:
        print("Ошибка: файл integrate_benchmark.txt не найден!")
        print("Сначала запустите ./integrate в папке build")
        return
    
    threads = []
    speedup = []
    
    with open(filename, 'r') as f:
        for line in f:
            if line.startswith('#'):
                continue
            parts = line.strip().split()
            if len(parts) >= 3:
                threads.append(int(parts[0]))
                speedup.append(float(parts[2]))
    
    threads, speedup = zip(*sorted(zip(threads, speedup)))
    
    plt.figure(figsize=(10, 6))
    plt.plot(threads, speedup, 'o-', linewidth=2, markersize=8, label='Ускорение')
    plt.plot([1, max(threads)], [1, max(threads)], 'k--', linewidth=2, label='Линейное ускорение')
    
    plt.xlabel('Количество потоков', fontsize=14)
    plt.ylabel('Ускорение', fontsize=14)
    plt.title('Ускорение численного интегрирования\nnsteps = 40000000', fontsize=16)
    plt.grid(True, linestyle=':', alpha=0.7)
    plt.legend(fontsize=12)
    plt.xticks(threads)
    
    plt.tight_layout()
    plt.savefig('integrate_speedup.png', dpi=150)
    plt.show()
    
    print(f"График сохранён в integrate_speedup.png")

if __name__ == '__main__':
    main()