#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Отдельный исполняемый файл, который запускает launcher_execlp.
   Не включается в launcher и не вызывается как обычная функция. */

/* getenv возвращает указатель на значение строки окружения,
   а если переменной нет — NULL. Не путайте NULL с пустой строкой. */
static void print_env(const char *name) {
    const char *value = getenv(name);
    printf(" target: %s=%s\n", name, value == NULL ? "(unset)" : value);
}

int main(int argc, char *argv[]) {
    /* Сопоставьте PID с PID ребёнка из launcher перед exec.
       Они совпадают: новый процесс при exec не создаётся. */
    printf(" target: PID=%ld PPID=%ld\n", (long)getpid(), (long)getppid());

    /* Аргументы пришли от launcher через список или массив exec.
       Печатаем каждый, включая argv[0], чтобы увидеть реальный argv. */
    printf(" target: argc=%d\n", argc);
    for (int i = 0; i < argc; ++i) {
        printf(" target: argv[%d]=%s\n", i, argv[i]);
    }

    /* Окружение — другой канал данных. Сравните вывод с командами
       из README: эти значения унаследованы от вызывающего процесса. */
    print_env("LESSON_LABEL");
    print_env("PARENT_ONLY");
    print_env("PATH");

    return EXIT_SUCCESS;
}
