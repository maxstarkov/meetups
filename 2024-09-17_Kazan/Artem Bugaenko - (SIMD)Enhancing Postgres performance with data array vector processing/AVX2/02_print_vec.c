#pragma GCC target("avx2")
#include <immintrin.h>
#include <stdio.h>

// Функция для вывода содержимого AVX2 регистра (__m256i) в виде массива 32-разрядных целых чисел
void print_avx2_register(const char *pname, __m256i r) {
    int vals[8] __attribute__((aligned(32)));

    // Сохранение содержимого регистра в массив
    _mm256_store_si256((__m256i*)vals, r);

    // Вывод содержимого массива
    printf("Содержимое регистра %s:\n", pname);
    for (int i = 0; i < 8; i++) {
        printf("%s[%d] = %d\n", pname, i, vals[i]);
    }
}

int main() {
    // Используем _mm256_set_epi32 для задания значений в векторе
    __m256i vec = _mm256_set_epi32(8, 7, 6, 5, 4, 3, 2, 1);

    // Вывод содержимого регистра
    print_avx2_register("vec", vec);

    return 0;
}
