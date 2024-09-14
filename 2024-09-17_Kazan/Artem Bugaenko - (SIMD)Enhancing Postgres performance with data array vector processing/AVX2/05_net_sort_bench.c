#pragma GCC target("avx2")
#include <immintrin.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>


#if defined(_WIN32) || defined(_WIN64)
#include <malloc.h>

int aligned_alloc_array(size_t alignment, size_t size, void** array) {
    *array = _aligned_malloc(size, alignment);
    if (*array == NULL) {
        return -1; // Ошибка выделения памяти
    }
    return 0; // Успех
}

void aligned_free(void* ptr) {
    _aligned_free(ptr); // Windows освобождение
}
#else
#include <stdlib.h>

int aligned_alloc_array(size_t alignment, size_t size, void** array) {
    int result = posix_memalign(array, alignment, size);
    if (result != 0) {
        return -1; // Ошибка выделения памяти
    }
    return 0; // Успех
}

void aligned_free(void* ptr) {
    free(ptr); // Linux/Unix освобождение
}
#endif

#define REPIT 1000000
#define SIZE 8

unsigned long rdtsc() {
    unsigned int lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((unsigned long)hi << 32) | lo;
}

// Функция для проверки, отсортирован ли массив по возрастанию
bool is_sorted(int* data, int size) {
    for (int i = 1; i < size; i++) {
        if (data[i - 1] > data[i]) {
            return false;
        }
    }
    return true;
}
// Кроссплатформенное выравнивание для Windows и Linux

#define ALIGNED_32 __attribute__((aligned(32)))


// Выровненные глобальные константы для перестановок
ALIGNED_32  int permute_step_1_1[8] = {1, 0, 3, 2, 5, 4, 7, 6};
ALIGNED_32  int permute_step_2_1[8] = {3, 2, 1, 0, 7, 6, 5, 4};
ALIGNED_32  int permute_step_2_2[8] = {1, 0, 3, 2, 5, 4, 7, 6};
ALIGNED_32  int permute_step_3_1[8] = {7, 6, 5, 4, 3, 2, 1, 0};
ALIGNED_32  int permute_step_3_2[8] = {2, 3, 0, 1, 6, 7, 4, 5};
ALIGNED_32  int permute_step_3_3[8] = {1, 0, 3, 2, 5, 4, 7, 6};

// Функция для сортировки блоков по 8 элементов с использованием битонической сортировки
void bitonic_sort_8(int* data) {
    // Используем выровненные команды для загрузки данных
    __m256i data_vec = _mm256_load_si256((__m256i*)data); // Выравненная загрузка

    // Шаг 1: перестановка и минимизация/максимизация
    __m256i perm = _mm256_load_si256((__m256i*)permute_step_1_1);
    __m256i exch = _mm256_permutevar8x32_epi32(data_vec, perm);
    __m256i vmin = _mm256_min_epi32(data_vec, exch);
    __m256i vmax = _mm256_max_epi32(data_vec, exch);
    data_vec = _mm256_blend_epi32(vmin, vmax, 0b10101010);

    // Шаг 2
    perm = _mm256_load_si256((__m256i*)permute_step_2_1);
    exch = _mm256_permutevar8x32_epi32(data_vec, perm);
    vmin = _mm256_min_epi32(data_vec, exch);
    vmax = _mm256_max_epi32(data_vec, exch);
    data_vec = _mm256_blend_epi32(vmin, vmax, 0b11001100);

    perm = _mm256_load_si256((__m256i*)permute_step_2_2);
    exch = _mm256_permutevar8x32_epi32(data_vec, perm);
    vmin = _mm256_min_epi32(data_vec, exch);
    vmax = _mm256_max_epi32(data_vec, exch);
    data_vec = _mm256_blend_epi32(vmin, vmax, 0b10101010);

    // Шаг 3
    perm = _mm256_load_si256((__m256i*)permute_step_3_1);
    exch = _mm256_permutevar8x32_epi32(data_vec, perm);
    vmin = _mm256_min_epi32(data_vec, exch);
    vmax = _mm256_max_epi32(data_vec, exch);
    data_vec = _mm256_blend_epi32(vmin, vmax, 0b11110000);

    perm = _mm256_load_si256((__m256i*)permute_step_3_2);
    exch = _mm256_permutevar8x32_epi32(data_vec, perm);
    vmin = _mm256_min_epi32(data_vec, exch);
    vmax = _mm256_max_epi32(data_vec, exch);
    data_vec = _mm256_blend_epi32(vmin, vmax, 0b11001100);

    perm = _mm256_load_si256((__m256i*)permute_step_3_3);
    exch = _mm256_permutevar8x32_epi32(data_vec, perm);
    vmin = _mm256_min_epi32(data_vec, exch);
    vmax = _mm256_max_epi32(data_vec, exch);
    data_vec = _mm256_blend_epi32(vmin, vmax, 0b10101010);

    // Сохранение отсортированных данных
    _mm256_store_si256((__m256i*)data, data_vec);
}

// Функция слияния двух отсортированных частей массива
void merge(int* data, int left, int mid, int right) {
    // Проверяем, отсортированы ли уже блоки
    if (data[mid] <= data[mid + 1]) {
        // Блоки уже отсортированы, слияние не нужно
        return;
    }

    // Выполняем обычное слияние, если блоки не отсортированы
    int n1 = mid - left + 1;
    int n2 = right - mid;

    int L[n1], R[n2];

    for (int i = 0; i < n1; i++)
        L[i] = data[left + i];
    for (int j = 0; j < n2; j++)
        R[j] = data[mid + 1 + j];

    int i = 0, j = 0, k = left;
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            data[k] = L[i];
            i++;
        } else {
            data[k] = R[j];
            j++;
        }
        k++;
    }

    while (i < n1) {
        data[k] = L[i];
        i++;
        k++;
    }

    while (j < n2) {
        data[k] = R[j];
        j++;
        k++;
    }
}

// Функция для выполнения сортировки слиянием
void merge_sort(int* data, int left, int right) {
    if (left < right) {
        int mid = left + (right - left) / 2;

        // Рекурсивная сортировка
        merge_sort(data, left, mid);
        merge_sort(data, mid + 1, right);

        // Слияние отсортированных частей
        merge(data, left, mid, right);
    }
}

void hybrid_sort(int* data, int size) {
    // Применяем битоническую сортировку к каждому блоку по 8 элементов
    for (int i = 0; i < size; i += 8) {
        if (i + 8 <= size) {
            bitonic_sort_8(&data[i]);
        }
    }
}
void sisd_sort(int* data, int size) {
    // Применяем сортировку слиянием
    merge_sort(data, 0, size - 1);
}

void initialize_array(int* array, int size) {
    for (int i = 0; i < size; ++i) {
        array[i] = rand() % 1000; // Случайные числа от 0 до 9999
    }
}

double benchmark(void (*func)( int*, int),  int* array, int size, int repetitions) {
    int* data;

    if (aligned_alloc_array(32, SIZE * sizeof(int), (void**)&data) != 0 ) {  // Linux/Unix
        printf("Memory allocation error.\n");
        return 1;
    }

    double acc =0;
    unsigned long acct =0;
    unsigned long startt=0;
    unsigned long endt =0;
    for (int i = 0; i < repetitions; ++i) {
        for (int j = 0; j < SIZE; j++) {
            data[j] = array[j];
        }
        clock_t start = clock();
         startt = rdtsc();
        func(data, size);
        endt = rdtsc();
        clock_t end = clock();
        acct+=endt - startt;
        acc+=(double)(end - start) / CLOCKS_PER_SEC;
    }

    printf("tacts: %lu\n", acct/REPIT);


    for (int i = 0; i < SIZE; i++) {
        array[i] = data[i];
    }
    aligned_free(data);
    return acc;
}

int main() {
    srand(time(NULL));
    const size_t alignment = 32;  // Выравнивание на 32 байта для AVX2
    // Выделение выровненной памяти для двух массивов
    int* data;
    int* data2;

    if (aligned_alloc_array(alignment, SIZE * sizeof(int), (void**)&data) != 0 ||
        aligned_alloc_array(alignment, SIZE * sizeof(int), (void**)&data2) != 0) {  // Linux/Unix
        printf("Memory allocation error.\n");
        return 1;
    }

    // Инициализация массива
    initialize_array(data, SIZE);

    // Копирование массива data в data2
    for (int i = 0; i < SIZE; i++) {
        data2[i] = data[i];
    }

    // Запуск бенчмарков
    printf("Measuring %d reps of finds in %d-sized array\n", REPIT, SIZE);

    // Бенчмарк для SISD
    double time_sisd = benchmark(sisd_sort, data, SIZE, REPIT);
    printf("Measured (SISD): %f seconds\n\n", time_sisd);
    // Проверка, отсортирован ли массив
    if (is_sorted(data, SIZE)) {
        printf("SISD SORTED.\n");
    } else {
        printf("SISD Sort FAIL!\n");
    }

    // Бенчмарк для SIMD
    double time_simd = benchmark(hybrid_sort, data2, SIZE, REPIT);
    printf("Measured (SIMD): %f seconds\n\n", time_simd);
    // Проверка, отсортирован ли массив
    if (is_sorted(data2, SIZE)) {
        printf("SIMD SORTED.\n");
    } else {
        printf("Sort FAIL!\n");
    }

    if (time_simd > 0) {
        printf("SIMD speedup against SISD = %.2f \n", time_sisd / time_simd);
    } else {
        printf(" SIMD time is 0!\n\n");
    }
    // Освобождаем выровненную память
    aligned_free(data);
    aligned_free(data2);

    return 0;
}
