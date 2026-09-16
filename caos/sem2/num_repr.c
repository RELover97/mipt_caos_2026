#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <stdbool.h>

// Печать двоичного представления числа n в bits битах
void print_binary(uint32_t n, int bits, _Bool print_sep) {
    for (int i = bits - 1; i >= 0; i--) {
        printf("%d", (n >> i) & 1);
        if (print_sep) {
            if (i == 31 || i == 23) printf(" "); // Разделители для наглядности
        }
    }
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
    
    return 0;
}