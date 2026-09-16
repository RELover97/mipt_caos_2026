#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <stdbool.h>

// Объединение позволяет смотреть на одни и те же данные как на float и как на набор бит
typedef union {
    float f;
    uint32_t u;
} FloatBits;

// Печать двоичного представления числа n в bits битах
void print_binary(uint32_t n, int bits, _Bool print_sep) {
    for (int i = bits - 1; i >= 0; i--) {
        printf("%d", (n >> i) & 1);
        if (print_sep) {
            if (i == 31 || i == 23) printf(" "); // Разделители для наглядности
        }
    }
}

// Разобрать двоичное представление вещественного числа
void analyze_float(float num) {
    FloatBits fb;
    fb.f = num;

    // Выделяем части числа согласно IEEE 754
    // 1 бит знака, 8 бит экспоненты, 23 бита мантиссы
    uint32_t sign = (fb.u >> 31) & 0x1;
    uint32_t exponent = (fb.u >> 23) & 0xFF;
    uint32_t mantissa = fb.u & 0x7FFFFF;

    printf("Число: %f\n", num);
    printf("HEX:   0x%08X\n", fb.u);
    printf("Bits:  ");
    print_binary(fb.u, 32, true);
    printf("\n");

    printf("|- Знак:      %u (%s)\n", sign, sign ? "минус" : "плюс");
    printf("|- Экспонента: %u (в коде: %d)\n", exponent, (int)exponent - 127);
    printf("|- Мантисса:   0x%06X\n", mantissa);
    printf("------------------------------------\n\n");
}

// Разобрать двоичное представление знакового целого числа
void analyze_int(int32_t num)
{
    printf("Число: %d\n", num);
    printf("HEX:   0x%08X\n", num);
    printf("Bits:  ");
    print_binary(num, 32, false);
    printf("\n");
}

// Разобрать двоичное представление беззнакового целого числа
void analyze_uint(uint32_t num)
{
    printf("Число: %u\n", num);
    printf("HEX:   0x%08X\n", num);
    printf("Bits:  ");
    print_binary(num, 32, false);
    printf("\n");
}


int main() 
{
    printf("Print signed integers:\n");
    analyze_int(0);
    analyze_int(1);
    analyze_int(-1);
    analyze_int(INT_MAX);
    analyze_int(INT_MIN);

    printf("Print unsigned integers:\n");
    analyze_uint(0);
    analyze_uint(1);
    analyze_uint(-1);
    analyze_uint(UINT_MAX);
    // analyze_uint(UINT_MIN); such constant is not defined

    printf("Print floats:\n");
    analyze_float(0.0f); // +0.0 and -0.0
    analyze_float(1.0f);
    analyze_float(0.1f); // periodic
    analyze_float(16777216.0f); // ULP = 2.0
    analyze_float(16777217.0f); // same as above
    analyze_float(16777218.0f);
    analyze_float(-10.125);
    
    return 0;
}