#include <stdio.h>

/* Флаги -O0 -g3 из Makefile позволяют удобно находить символы.
   Расположение объектов по секциям определяет компилятор/компоновщик:
   следующие комментарии — типичный случай, а не гарантия языка C. */
int initialized_data = 7;            /* Ненулевая инициализация: обычно .data. */
int uninitialized_data[256];         /* Нулевая инициализация: обычно .bss. */
const char constant_text[] = "ELF";  /* Константа: обычно данные только для чтения, .rodata. */

/* Именованную функцию удобно искать через objdump -d. */
int calculate(void) {
    return initialized_data + uninitialized_data[0];
}

int main(void) {
    /* Точка входа ELF обычно расположена в коде запуска (_start),
       а не непосредственно в main(): runtime вызывает main позже. */
    printf("ELF: %s, result=%d\n", constant_text, calculate());
    return 0;
}
