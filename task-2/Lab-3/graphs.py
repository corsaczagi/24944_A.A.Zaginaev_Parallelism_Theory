#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import matplotlib.pyplot as plt
import os

def read_results(filename='results.txt'):
    """Чтение результатов из файла"""
    possible_paths = ['results.txt', 'build/results.txt', '../build/results.txt', './build/results.txt']
    
    found_path = None
    for path in possible_paths:
        if os.path.exists(path):
            found_path = path
            print(f"Найден файл: {found_path}")
            break
    
    if found_path is None:
        print("Ошибка: файл results.txt не найден!")
        return [], [], [], [], [], []
    
    threads = []
    serial_times = []
    split_times = []
    single_times = []
    speedup_split = []
    speedup_single = []
    
    with open(found_path, 'r') as f:
        for line in f:
            if line.startswith('#'):
                continue
            parts = line.strip().split()
            if len(parts) >= 6:
                threads.append(int(parts[0]))
                serial_times.append(float(parts[1]))
                split_times.append(float(parts[2]))
                single_times.append(float(parts[3]))
                speedup_split.append(float(parts[4]))
                speedup_single.append(float(parts[5]))
    
    return threads, serial_times, split_times, single_times, speedup_split, speedup_single

def plot_all_in_one(threads, serial_times, split_times, single_times, speedup_split, speedup_single):
    """Все графики в одном файле (3 подграфика)"""
    
    fig, axes = plt.subplots(1, 3, figsize=(18, 5))
    fig.suptitle('Анализ производительности метода простой итерации\n(размер матрицы 2000×2000)', fontsize=16, fontweight='bold')
    
    # График 1: Время выполнения
    ax1 = axes[0]
    serial_time = serial_times[0] if serial_times else 0
    ax1.plot(threads, [serial_time] * len(threads), 'k--', linewidth=2, 
             label=f'Serial ({serial_time:.2f} с)', marker='s')
    ax1.plot(threads, split_times, 'o-', linewidth=2, markersize=8, 
             label='OMP Split', color='blue')
    ax1.plot(threads, single_times, 's-', linewidth=2, markersize=8, 
             label='OMP Single Region', color='red')
    ax1.set_xlabel('Количество потоков', fontsize=12)
    ax1.set_ylabel('Время (секунды)', fontsize=12)
    ax1.set_title('Время выполнения', fontsize=14, fontweight='bold')
    ax1.grid(True, linestyle=':', alpha=0.7)
    ax1.legend(fontsize=10)
    ax1.set_xticks(threads)
    
    # График 2: Ускорение
    ax2 = axes[1]
    ax2.plot(threads, speedup_split, 'o-', linewidth=2, markersize=8, 
             label='OMP Split', color='blue')
    ax2.plot(threads, speedup_single, 's-', linewidth=2, markersize=8, 
             label='OMP Single Region', color='red')
    ax2.plot(threads, threads, 'k--', linewidth=2, 
             label='Идеальное ускорение')
    ax2.set_xlabel('Количество потоков', fontsize=12)
    ax2.set_ylabel('Ускорение (Speedup)', fontsize=12)
    ax2.set_title('Ускорение', fontsize=14, fontweight='bold')
    ax2.grid(True, linestyle=':', alpha=0.7)
    ax2.legend(fontsize=10)
    ax2.set_xticks(threads)
    
    # Добавляем значения на график ускорения
    for i, (t, s_split, s_single) in enumerate(zip(threads, speedup_split, speedup_single)):
        ax2.annotate(f'{s_split:.2f}', (t, s_split), textcoords="offset points", 
                     xytext=(0, 10), ha='center', fontsize=8, color='blue')
        ax2.annotate(f'{s_single:.2f}', (t, s_single), textcoords="offset points", 
                     xytext=(0, -15), ha='center', fontsize=8, color='red')
    
    # График 3: Эффективность
    ax3 = axes[2]
    efficiency_split = [s/t for s, t in zip(speedup_split, threads)]
    efficiency_single = [s/t for s, t in zip(speedup_single, threads)]
    
    ax3.plot(threads, efficiency_split, 'o-', linewidth=2, markersize=8, 
             label='OMP Split', color='blue')
    ax3.plot(threads, efficiency_single, 's-', linewidth=2, markersize=8, 
             label='OMP Single Region', color='red')
    ax3.plot([1, max(threads)], [1, 1], 'k--', linewidth=2, 
             label='Идеальная эффективность (100%)')
    ax3.set_xlabel('Количество потоков', fontsize=12)
    ax3.set_ylabel('Эффективность', fontsize=12)
    ax3.set_title('Эффективность распараллеливания', fontsize=14, fontweight='bold')
    ax3.grid(True, linestyle=':', alpha=0.7)
    ax3.legend(fontsize=10)
    ax3.set_xticks(threads)
    ax3.set_ylim(0, 1.1)
    
    plt.tight_layout()
    plt.savefig('all_plots.png', dpi=150, bbox_inches='tight')
    plt.savefig('all_plots.pdf', bbox_inches='tight')
    print("Все графики сохранены в all_plots.png и all_plots.pdf")
    plt.show()

def plot_separate(threads, serial_times, split_times, single_times, speedup_split, speedup_single):
    """Три отдельных графика в разных файлах"""
    
    # График 1: Время
    plt.figure(figsize=(10, 6))
    serial_time = serial_times[0] if serial_times else 0
    plt.plot(threads, [serial_time] * len(threads), 'k--', linewidth=2, 
             label=f'Serial ({serial_time:.2f} с)', marker='s')
    plt.plot(threads, split_times, 'o-', linewidth=2, markersize=8, label='OMP Split')
    plt.plot(threads, single_times, 's-', linewidth=2, markersize=8, label='OMP Single Region')
    plt.xlabel('Количество потоков', fontsize=14)
    plt.ylabel('Время (секунды)', fontsize=14)
    plt.title('Время выполнения метода простой итерации', fontsize=16)
    plt.grid(True, linestyle=':', alpha=0.7)
    plt.legend(fontsize=12)
    plt.xticks(threads)
    plt.tight_layout()
    plt.savefig('time_comparison.png', dpi=150)
    plt.close()
    
    # График 2: Ускорение
    plt.figure(figsize=(10, 6))
    plt.plot(threads, speedup_split, 'o-', linewidth=2, markersize=8, label='OMP Split')
    plt.plot(threads, speedup_single, 's-', linewidth=2, markersize=8, label='OMP Single Region')
    plt.plot(threads, threads, 'k--', linewidth=2, label='Идеальное ускорение')
    plt.xlabel('Количество потоков', fontsize=14)
    plt.ylabel('Ускорение (Speedup)', fontsize=14)
    plt.title('Ускорение от количества потоков', fontsize=16)
    plt.grid(True, linestyle=':', alpha=0.7)
    plt.legend(fontsize=12)
    plt.xticks(threads)
    plt.tight_layout()
    plt.savefig('speedup_comparison.png', dpi=150)
    plt.close()
    
    # График 3: Эффективность
    plt.figure(figsize=(10, 6))
    efficiency_split = [s/t for s, t in zip(speedup_split, threads)]
    efficiency_single = [s/t for s, t in zip(speedup_single, threads)]
    plt.plot(threads, efficiency_split, 'o-', linewidth=2, markersize=8, label='OMP Split')
    plt.plot(threads, efficiency_single, 's-', linewidth=2, markersize=8, label='OMP Single Region')
    plt.plot([1, max(threads)], [1, 1], 'k--', linewidth=2, label='Идеальная эффективность')
    plt.xlabel('Количество потоков', fontsize=14)
    plt.ylabel('Эффективность', fontsize=14)
    plt.title('Эффективность распараллеливания', fontsize=16)
    plt.grid(True, linestyle=':', alpha=0.7)
    plt.legend(fontsize=12)
    plt.xticks(threads)
    plt.ylim(0, 1.1)
    plt.tight_layout()
    plt.savefig('efficiency_comparison.png', dpi=150)
    plt.close()
    
    print("Графики сохранены в отдельных файлах:")
    print("  - time_comparison.png")
    print("  - speedup_comparison.png")
    print("  - efficiency_comparison.png")

def main():
    threads, serial_times, split_times, single_times, speedup_split, speedup_single = read_results()
    
    if not threads:
        return
    
    print(f"Найдены данные для {len(threads)} конфигураций потоков")
    print(f"Потоки: {threads}")
    print(f"Ускорение Split: {[round(s, 2) for s in speedup_split]}")
    print(f"Ускорение Single: {[round(s, 2) for s in speedup_single]}")
    print()
    
    # Сохраняем ВСЕ ГРАФИКИ В ОДНОМ ФАЙЛЕ
    plot_all_in_one(threads, serial_times, split_times, single_times, speedup_split, speedup_single)
    
    # Выводы
    print("\n=== ВЫВОДЫ ===")
    print(f"1. Последовательное время: {serial_times[0]:.2f} секунд")
    print(f"2. Лучшее ускорение Split: {max(speedup_split):.2f}x на {threads[speedup_split.index(max(speedup_split))]} потоках")
    print(f"3. Лучшее ускорение Single: {max(speedup_single):.2f}x на {threads[speedup_single.index(max(speedup_single))]} потоках")
    
    if max(speedup_single) > max(speedup_split):
        print("4. Вариант Single Region показал лучшее ускорение")
    else:
        print("4. Вариант Split показал лучшее ускорение")
    
    print("\n5. Рекомендация: Использовать OMP Single Region со schedule(static)")

if __name__ == '__main__':
    main()
