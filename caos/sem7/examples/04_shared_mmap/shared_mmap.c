#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
    /* Контрольный объект: обычная локальная переменная после fork() не общая. */
    int private_value = 10;

    /* Создаём отображение ДО fork(): потомок унаследует тот же общий объект.
       NULL: адрес выбирает ядро; PROT_*: права чтения/записи;
       MAP_SHARED: запись видна обоим процессам;
       MAP_ANONYMOUS: нет обычного файла, поэтому fd=-1 и offset=0. */
    int *shared_value = mmap(NULL, sizeof(*shared_value), PROT_READ | PROT_WRITE,
                             MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shared_value == MAP_FAILED) { /* Ошибка mmap обозначается MAP_FAILED, а не NULL. */
        perror("mmap");
        return EXIT_FAILURE;
    }
    *shared_value = 10;
    fflush(stdout);

    pid_t child = fork();
    if (child == -1) {
        perror("fork");
        munmap(shared_value, sizeof(*shared_value));
        return EXIT_FAILURE;
    }
    if (child == 0) {
        /* Только изменение общего отображения увидит родитель.
           Указатели в разных процессах могут иметь одинаковые виртуальные
           адреса, но это само по себе НЕ означает общую память. */
        private_value = 99;
        *shared_value = 42;
        printf("child: private=%d shared=%d\n", private_value, *shared_value);
        fflush(stdout);
        _exit(EXIT_SUCCESS);
    }

    /* MAP_SHARED обеспечивает ВИДИМОСТЬ данных, но не порядок действий.
       waitpid гарантирует, что ребёнок закончил запись до чтения
       родителем. Конкурентного доступа здесь нет. */
    if (waitpid(child, NULL, 0) == -1) {
        perror("waitpid");
        munmap(shared_value, sizeof(*shared_value));
        return EXIT_FAILURE;
    }
    printf("parent: private=%d shared=%d\n", private_value, *shared_value);

    /* Удаляем отображение из адресного пространства данного процесса. */
    if (munmap(shared_value, sizeof(*shared_value)) == -1) {
        perror("munmap");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
