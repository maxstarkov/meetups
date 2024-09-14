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

// Функция для инициализации массива случайными числами
void initialize_array(int* array, int size) {
    for (int i = 0; i < size; ++i) {
        array[i] = rand() % 10000; // Случайные числа от 0 до 9999
    }
}

// Функция для поиска максимума с использованием SISD (обычный подход)
int find_max_sisd(const int* array, int size) {
    int max_value = array[0];
    for (int i = 1; i < size; ++i) {
        if (array[i] > max_value) {
            max_value = array[i];
        }
    }
    return max_value;
}

// Функция для поиска максимума с использованием AVX2 (SIMD)
int find_max_simd(const int* array, int size) {
    __m256i max_vec = _mm256_set1_epi32(array[0]);
    
    int i = 0;
    for (; i <= size - 8; i += 8) {
        __m256i vec = _mm256_loadu_si256((__m256i*)&array[i]);
        max_vec = _mm256_max_epi32(max_vec, vec);
    }
    
    int max_vals[8];
    _mm256_storeu_si256((__m256i*)max_vals, max_vec);

    int max_value = max_vals[0];
    for (int j = 1; j < 8; ++j) {
        if (max_vals[j] > max_value) {
            max_value = max_vals[j];
        }
    }

    for (; i < size; ++i) {
        if (array[i] > max_value) {
            max_value = array[i];
        }
    }
    
    return max_value;
}


// Универсальная функция для бенчмаркинга
double benchmark(int (*func)(const int*, int), int* array, int size, int repetitions) {
    double acc = 0;      // Переменная для накопления времени исполнения относительно time.h
    uint64_t acct = 0;   // Переменная для замера среднего количества тактов (TSC) на одну итерацию
    uint64_t startt = 0; // Переменная для значения (TSC) до вызова функции
    uint64_t endt = 0;   // Переменная для значения (TSC) после вызова функции

    for (int i = 0; i < repetitions; ++i) {
        initialize_array(array, size); // Чтобы на каждой итерации работать с новым массивом
        uint64_t start_time = start_timer();   // Старт высокоточного замера времени
        startt = __rdtsc();                    // Начало замера тактов

        func(array, size);       // Вызов функции

        endt = __rdtsc();                      // Окончание замера тактов
        uint64_t elapsed_time_ns = end_timer(start_time);  // Окончание замера времени
        acct += endt - startt;  // Накопление тактов за итерацию
        acc += (double)elapsed_time_ns / 1000000000.0;  // Накопление времени в секундах
    }
    printf("Tacts for one repetition: %lu\n", acct / repetitions);
    return acc;
}


int main() {
    int data[SIZE];  // Статический массив
    printf("Measuring %d reps of finds max in %d-sized array\n", REPIT, SIZE);
    // Инициализация случайными числами
    srand(time(NULL));
    initialize_array(data, SIZE);

    // Бенчмарк для SISD
    double time_sisd = benchmark(find_max_sisd, data, SIZE, REPIT);
    printf("Measured (SISD): %f seconds\n\n", time_sisd);

    // Бенчмарк для SIMD
    double time_simd = benchmark(find_max_simd, data, SIZE, REPIT);
    printf("Measured (SIMD): %f seconds\n\n", time_simd);
    if (time_simd > 0) {
        printf("SIMD speedup against SISD = %.2f \n\n", time_sisd / time_simd);
    } else {
        printf(" SIMD time is 0!\n\n");
    }
    return 0;
}

