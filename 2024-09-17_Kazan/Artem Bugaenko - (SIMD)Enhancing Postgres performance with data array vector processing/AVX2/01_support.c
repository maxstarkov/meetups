#include <stdio.h>

void cpuid(int info[4], int InfoType, int Subleaf) {
    __asm__ __volatile__ (
            "cpuid" :
            "=a" (info[0]), "=b" (info[1]), "=c" (info[2]), "=d" (info[3]) :
            "a" (InfoType), "c" (Subleaf)  // Передаем и InfoType, и Subleaf
            );
}

int main() {
    int info[4];

    // Получаем информацию о поддержке инструкций
    cpuid(info, 1, 0);
    int ecx = info[2];
    int edx = info[3];

    cpuid(info, 7, 0);
    int ebx = info[1];
    // Проверка поддержки различных инструкций
    if (edx & (1 << 25)) printf("SSE supported\n");
    if (edx & (1 << 26)) printf("SSE2 supported\n");
    if (ecx & (1 << 0))  printf("SSE3 supported\n");
    if (ecx & (1 << 9))  printf("SSSE3 supported\n");
    if (ecx & (1 << 19)) printf("SSE4.1 supported\n");
    if (ecx & (1 << 20)) printf("SSE4.2 supported\n");
    if (ecx & (1 << 28)) printf("AVX supported\n");
    if (ebx & (1 << 5))  printf("AVX2 supported\n");
    if (ebx & (1 << 16)) printf("AVX-512F supported\n");

    return 0;
}
