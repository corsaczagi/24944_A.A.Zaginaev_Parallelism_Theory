## Выбор типа массива

Тип массива выбирается во время сборки через CMake-опцию `USE_DOUBLE`.

### Сборка с float (по умолчанию)

```bash
cmake -DUSE_DOUBLE=OFF ..
cmake --build .

```

### Сборка с float

```bash
cmake -DUSE_DOUBLE=ON ..
cmake --build .
```

### Результаты выполнения

double:
``` bash
Type: double
Sum = 4.89582e-11
```
float:
```bash
Type: float
Sum = 0.291951
```

### Проверка коммита
