#!/usr/bin/env python3
import matplotlib.pyplot as plt
import numpy as np

def main():
    # Чтение данных
    threads = []
    time_v1 = []
    time_v2 = []
    speedup_v1 = []
    speedup_v2 = []
    
    with open('slau_benchmark.txt', 'r') as f:
        for line in f:
            if line.startswith('#'):
                continue
            parts = line.strip().split()
            if len(parts) >= 5:
                threads.append(int(parts[0]))
                time_v1.append(float(parts[1]))
                time_v2.append(float(parts[2]))
                speedup_v1.append(float(parts[3]))
                speedup_v2.append(float(parts[4]))
    
    # График времени
    plt.figure(figsize=(12, 5))
    
    plt.subplot(1, 2, 1)
    plt.plot(threads, time_v1, 'o-', label='Вариант 1 (parallel for)', linewidth=2)
    plt.plot(threads, time_v2, 's-', label='Вариант 2 (parallel region)', linewidth=2)
    plt.xlabel('Количество потоков')
    plt.ylabel('Время (сек)')
    plt.title('Время выполнения')
    plt.grid(True, linestyle=':', alpha=0.7)
    plt.legend()
    plt.xticks(threads)
    
    # График ускорения
    plt.subplot(1, 2, 2)
    plt.plot(threads, speedup_v1, 'o-', label='Вариант 1', linewidth=2)
    plt.plot(threads, speedup_v2, 's-', label='Вариант 2', linewidth=2)
    plt.plot([1, max(threads)], [1, max(threads)], 'k--', label='Идеальное', linewidth=2)
    plt.xlabel('Количество потоков')
    plt.ylabel('Ускорение')
    plt.title('Ускорение')
    plt.grid(True, linestyle=':', alpha=0.7)
    plt.legend()
    plt.xticks(threads)
    
    plt.tight_layout()
    plt.savefig('slau_speedup.png', dpi=150)
    plt.show()
    
    # График эффективности
    plt.figure(figsize=(10, 6))
    efficiency_v1 = [s/t for s, t in zip(speedup_v1, threads)]
    efficiency_v2 = [s/t for s, t in zip(speedup_v2, threads)]
    
    plt.plot(threads, efficiency_v1, 'o-', label='Вариант 1', linewidth=2)
    plt.plot(threads, efficiency_v2, 's-', label='Вариант 2', linewidth=2)
    plt.xlabel('Количество потоков')
    plt.ylabel('Эффективность')
    plt.title('Эффективность распараллеливания')
    plt.grid(True, linestyle=':', alpha=0.7)
    plt.legend()
    plt.xticks(threads)
    plt.ylim(0, 1.1)
    
    plt.tight_layout()
    plt.savefig('slau_efficiency.png', dpi=150)
    plt.show()
    
    # График schedule
    schedule_types = []
    schedule_times = []
    
    with open('schedule_benchmark.txt', 'r') as f:
        for line in f:
            if line.startswith('#'):
                continue
            parts = line.strip().split()
            if len(parts) >= 2:
                schedule_types.append(parts[0])
                schedule_times.append(float(parts[1]))
    
    plt.figure(figsize=(10, 6))
    bars = plt.bar(schedule_types, schedule_times, color=['blue', 'green', 'orange', 'red'])
    plt.xlabel('Тип schedule')
    plt.ylabel('Время (сек)')
    plt.title('Сравнение schedule (8 потоков)')
    for bar, time in zip(bars, schedule_times):
        plt.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.1, 
                f'{time:.2f}', ha='center', va='bottom')
    plt.grid(True, linestyle=':', alpha=0.7, axis='y')
    plt.tight_layout()
    plt.savefig('schedule_comparison.png', dpi=150)
    plt.show()

if __name__ == '__main__':
    main()