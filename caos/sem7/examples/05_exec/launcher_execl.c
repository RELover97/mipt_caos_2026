#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

/* Эта программа демонстрирует только execl().
   Запускайте из корня проекта: make создаёт отдельный файл target_execl. */
int main(void) {
    const char *path = "./build/05_exec/target_execl";

    /* В родителе печатаем PID, чтобы отличить его от PID ребёнка.
       fflush нужен, чтобы при fork не продублировать буфер stdout. */
    printf("launcher: parent PID=%ld\n", (long)getpid());
    fflush(stdout);

    /* fork создаёт новый процесс. В родителе результат — PID ребёнка,
       в ребёнке — 0. Оба пока выполняют тот же машинный код launcher. */
    pid_t child = fork();
    if (child == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (child == 0) {
        printf("launcher: child PID=%ld before execl\n", (long)getpid());
        printf("launcher: exec path=%s\n", path);
        fflush(stdout); /* exec заменит и буфер stdout старой программы. */

        /* Путь к исполняемому файлу задан явно, поиск по PATH не нужен.
           Новая программа получит текущее окружение ребёнка. */
        /* Передаём аргументы по одному. Нулевой указатель нужного типа
           завершает вариативный список аргументов новой программы. */
        execl(path, "target_execl", "hello", "42", (char *)NULL);

        /* Если exec успешен, этой строки никогда не достигнем:
           теперь в процессе с тем же PID выполняется target. */
        perror("execl");
        _exit(127); /* Учебный код неудачного запуска; не сбрасываем stdio. */
    }

    /* Сюда приходит только родитель. Он не заменял свою программу.
       waitpid может ждать уже сейчас, пока ребёнок запускает target. */
    int status = 0;
    if (waitpid(child, &status, 0) == -1) {
        perror("waitpid");
        return EXIT_FAILURE;
    }
    if (!WIFEXITED(status)) {
        fprintf(stderr, "launcher: child did not exit normally\n");
        return EXIT_FAILURE;
    }

    /* Передаём оболочке код завершения ребёнка, чтобы она могла
       различить успешный запуск (0) и ошибку exec (127). */
    int code = WEXITSTATUS(status);
    printf("launcher: child %ld exit=%d\n", (long)child, code);
    return code;
}
