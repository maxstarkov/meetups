#pragma GCC target("avx2")
#include <immintrin.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdint.h>


#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <time.h>
#endif


#define REPIT (10000)
#define SIZE (4096)

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

// Кроссплатформенное выделение выровненной памяти
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

void merge_blocks(int* data, int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;

    // Временные массивы для хранения данных
    int L[n1], R[n2];

    for (int i = 0; i < n1; i++)
        L[i] = data[left + i];
    for (int i = 0; i < n2; i++)
        R[i] = data[mid + 1 + i];

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

void merge_sort_blocks(int* data, int size, int block_size) {
    for (int block_size_step = block_size; block_size_step < size; block_size_step *= 2) {
        for (int left = 0; left < size; left += 2 * block_size_step) {
            int mid = left + block_size_step - 1;
            int right = (left + 2 * block_size_step - 1 < size) ? left + 2 * block_size_step - 1 : size - 1;
            if (mid < right)
                merge_blocks(data, left, mid, right);
        }
    }
}


// Функция для сортировки массива большего размера
void hybrid_sort(int* data, int size) {
    // Применяем битоническую сортировку к каждому блоку по 8 элементов
    for (int i = 0; i < size; i += 8) {
        if (i + 8 <= size) {
            bitonic_sort_8(&data[i]);
        }
    }

    // Применяем сортировку слиянием для объединения отсортированных блоков
       merge_sort_blocks( data, size, 8);
}

void swap(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

// Функция для разделения массива
int partition(int* data, int low, int high) {
    int pivot = data[high]; // Последний элемент как опорный (pivot)
    int i = low - 1; // Индекс меньшего элемента

    for (int j = low; j < high; j++) {
        if (data[j] < pivot) {
            i++;
            swap(&data[i], &data[j]);
        }
    }
    swap(&data[i + 1], &data[high]); // Перемещаем опорный элемент на правильное место
    return (i + 1);
}

// Основная рекурсивная функция для быстрой сортировки
void quick_sort(int* data, int low, int high) {
    if (low < high) {
        int pi = partition(data, low, high); // Индекс опорного элемента

        // Рекурсивно сортируем элементы до и после разделения
        quick_sort(data, low, pi - 1);
        quick_sort(data, pi + 1, high);
    }
}

// Функция для вызова быстрой сортировки
void sisd_sort2(int* data, int size) {
    quick_sort(data, 0, size - 1);
}

void sisd_sort(int* data, int size) {
    // Применяем сортировку слиянием
    merge_sort(data, 0, size - 1);
}

void initialize_array(int* array, int size) {
    for (int i = 0; i < size; ++i) {
        array[i] = rand() % 100000; // Случайные числа от 0 до 9999
    }
}




double benchmark(void (*func)( int*, int),  int* array, int size, int repetitions) {
    int* data;

    if (aligned_alloc_array(32, SIZE * sizeof(int), (void**)&data) != 0 ) {  // Linux/Unix
        printf("memory allocation error.\n");
        return 1;
    }

    double acc =0;
    uint64_t acct =0;
    uint64_t startt=0;
    uint64_t endt =0;
    for (int i = 0; i < repetitions; ++i) {
        for (int j = 0; j < SIZE; j++) {
            data[j] = array[j];
        }
        initialize_array(data,size);
        uint64_t start_time = start_timer();   // Старт высокоточного замера времени
        startt = __rdtsc();                    // Начало замера тактов

        func(data, size);       // Вызов функции

        endt = __rdtsc();                      // Окончание замера тактов
        uint64_t elapsed_time_ns = end_timer(start_time);  // Окончание замера времени
        acct += endt - startt;  // Накопление тактов за итерацию
        acc += (double)elapsed_time_ns / 1000000000.0;  // Накопление времени в секундах
    }

    printf("tacts: %lu\n", acct / REPIT);


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
    printf("Measured (SISD): %f seconds\n", time_sisd);
    // Проверка, отсортирован ли массив
    if (is_sorted(data, SIZE)) {
        printf("SISD SORTED.\n\n");
    } else {
        printf("SISD Sort FAIL!\n\n");
    }

    // Копирование массива data2 в data
    for (int i = 0; i < SIZE; i++) {
        data[i] = data2[i];
    }
    // Бенчмарк для quicksort SISD
    double time_sisd2 = benchmark(sisd_sort2, data, SIZE, REPIT);
    printf("Measured (SISD_quicksort): %f seconds\n", time_sisd2);

    // Проверка, отсортирован ли массив
    if (is_sorted(data, SIZE)) {
        printf("SISD QUICK SORTED.\n\n");
    } else {
        printf("QUICK Sort FAIL!\n\n");
    }

    // Бенчмарк для SIMD
    double time_simd = benchmark(hybrid_sort, data2, SIZE, REPIT);
    printf("Measured (SIMD): %f seconds\n", time_simd);



    // Проверка, отсортирован ли массив
    if (is_sorted(data2, SIZE)) {
        printf("SIMD SORTED.\n\n");
    } else {
        printf("Sort FAIL!\n\n");
    }

    if (time_simd > 0) {
        printf("SIMD speedup against SISD = %.2f \n", time_sisd / time_simd);
    } else {
        printf(" SIMD time is 0!\n");
    }
        if (time_simd > 0) {
        printf("SIMD speedup against QUICKSORT SISD = %.2f \n", time_sisd2 / time_simd);
    } else {
        printf(" SIMD time is 0!\n");
    }


    // Освобождаем выровненную память
    aligned_free(data);
    aligned_free(data2);

    return 0;
}
