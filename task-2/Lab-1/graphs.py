#!/usr/bin/env python3
# plot_speedup.py

import matplotlib.pyplot as plt
import sys

def main():
    # Определяем имя файла с данными
    filename = sys.argv[1] if len(sys.argv) > 1 else 'build/benchmark_data.txt'

    # Читаем данные
    data_20k = {'threads': [], 'speedup': []}
    data_40k = {'threads': [], 'speedup': []}

    with open(filename, 'r') as f:
        for line in f:
            if line.startswith('#'):
                continue
            parts = line.strip().split()
            if len(parts) < 4:
                continue
            size = int(parts[0])
            threads = int(parts[1])
            speedup = float(parts[3])
            if size == 20000:
                data_20k['threads'].append(threads)
                data_20k['speedup'].append(speedup)
            elif size == 40000:
                data_40k['threads'].append(threads)
                data_40k['speedup'].append(speedup)

    # Сортируем по количеству потоков (на случай, если в файле не по порядку)
    for data in (data_20k, data_40k):
        if data['threads']:
            data['threads'], data['speedup'] = zip(*sorted(zip(data['threads'], data['speedup'])))

    # Построение графика
    plt.figure(figsize=(8, 6))
    plt.plot(data_20k['threads'], data_20k['speedup'], 'o-', label='20000×20000')
    plt.plot(data_40k['threads'], data_40k['speedup'], 's-', label='40000×40000')

    # Линейное ускорение (y = x)
    max_threads = max(data_20k['threads'] + data_40k['threads'])
    plt.plot([1, max_threads], [1, max_threads], 'k--', label='Линейное ускорение')

    plt.xlabel('Количество потоков')
    plt.ylabel('Ускорение')
    plt.title('Ускорение матрично-векторного умножения')
    plt.grid(True, linestyle=':', alpha=0.7)
    plt.legend()
    plt.xlim(0, max_threads + 1)
    plt.ylim(0, max_threads + 1)
    plt.xticks(data_20k['threads'])  # отметки по потокам

    plt.tight_layout()
    plt.savefig('speedup_plot.png', dpi=150)
    print("График сохранён в speedup_plot.png")
    # При желании можно показать на экране:
    # plt.show()

if __name__ == '__main__':
    main()