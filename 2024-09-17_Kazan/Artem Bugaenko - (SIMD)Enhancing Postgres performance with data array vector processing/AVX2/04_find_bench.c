#pragma GCC target("avx2")
#include <immintrin.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <time.h>
#endif

#define REPIT (10000)
#define SIZE (4096*16)// Размер массива

// Абстракция для начала замера времени (кроссплатформенный вариант)
uint64_t start_timer() {
#ifdef _WIN32
LARGE_INTEGER start;
QueryPerformanceCounter(&start);
return start.QuadPart;
#elif __linux__
struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    return (uint64_t)(start.tv_sec * 1000000000 + start.tv_nsec);  // Перевод в наносекунды
#endif
}

// Абстракция для получения времени окончания замера и подсчёта разницы
uint64_t end_timer(uint64_t start) {
#ifdef _WIN32
    LARGE_INTEGER end, frequency;
    QueryPerformanceCounter(&end);
    QueryPerformanceFrequency(&frequency);
    return (uint64_t)((end.QuadPart - start) * 1000000000 / frequency.QuadPart);  // В наносекундах
#elif __linux__
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    uint64_t end_ns = (uint64_t)(end.tv_sec * 1000000000 + end.tv_nsec);
    return end_ns - start;  // Разница во времени в наносекундах
#endif
}

int find_sisd(const int* array, int size, int key) {
    int i;
    for (i = 0; i < size; i++)
        if (array[i] == key)
            return i;
    return -1;
}

static inline int first_equal_yvalue(__m256i src1, __m256i src2) {
    __m256i cmp_result = _mm256_cmpeq_epi32(src1, src2);
    int mask = _mm256_movemask_ps(_mm256_castsi256_ps(cmp_result));
    if (mask != 0) {
        return __builtin_ctz(mask);//позиция первого ненулевого
    }
    return -1;
}

int find_simd(const int* array, int size, int key) {
    __m256i vec_i = _mm256_set1_epi32(key);
    int j = 0; // Для обработки векторизуемой части массива
    // векторная часть
    for (; j < size - 8; j += 8) {
        __m256i vec_a = _mm256_loadu_si256((__m256i*)&array[j]);
        int pos = first_equal_yvalue(vec_a, vec_i);
        if (pos != -1) {
            return j + pos;
        }
    }
    // Обработка "хвостика"
    for (; j < size; ++j) {
        if (array[j] == key) {
            return j;
        }
    }
    return -1;
}

void initialize_array(int* array, int size) {
    for (int i = 0; i < size; ++i) {
        array[i] = i;
    }
}

// Универсальная функция бенчмаркинга с поддержкой высокоточных таймеров
double benchmark(int (*func)(const int*, int, int), const int* array, int size, int repetitions) {
    double acc = 0;       // Переменная для накопления времени исполнения
    uint64_t acct = 0;    // Переменная для накопления тактов (TSC)
    uint64_t startt = 0;  // Переменная для значения (TSC) до вызова функции
    uint64_t endt = 0;    // Переменная для значения (TSC) после вызова функции
    volatile int key = 0;
    volatile int result = 0;

    for (int i = 0; i < repetitions; ++i) {
        key = rand() % size; // Выбираем новый ключ

        // Используем высокоточный таймер вместо clock()
        uint64_t start_time = start_timer();   // Старт высокоточного замера времени
        startt = __rdtsc();                    // Начало замера тактов

        result = func(array, size, key);       // Вызов функции

        endt = __rdtsc();                      // Окончание замера тактов
        uint64_t elapsed_time_ns = end_timer(start_time);  // Окончание замера времени
        acct += endt - startt;  // Накопление тактов за итерацию
        acc += (double)elapsed_time_ns / 1000000000.0;  // Накопление времени в секундах
    }

    printf("key= %i,find= %i,\n", key, result);
    printf("Tacts for one repetition: %lu\n", acct / repetitions);
    return acc;  // Возвращаем общее время в секундах
}

int main() {
    int data[SIZE];
    printf("Measuring %d reps of finds key in %d-sized array\n", REPIT, SIZE);
    srand(time(NULL));
    initialize_array(data, SIZE);//заполним массив попорядку
    
    // Бенчмарк для SISD
    double time_sisd = benchmark(find_sisd, data, SIZE, REPIT);
    printf("Measured (SISD): %f seconds\n\n", time_sisd);

    // Бенчмарк для SIMD
    double time_simd = benchmark(find_simd, data, SIZE, REPIT);
    printf("Measured (SIMD): %f seconds\n\n", time_simd);
    if (time_simd > 0) {
        printf("SIMD speedup against SISD = %.2f \n\n", time_sisd / time_simd);
    } else {
        printf(" SIMD time is 0!\n\n");
    }
    return 0;
}