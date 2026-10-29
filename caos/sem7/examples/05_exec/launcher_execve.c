#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

/* Эта программа демонстрирует только execve().
   Запускайте из корня проекта: make создаёт отдельный файл target_execve. */
int main(int argc, char *argv[]) {
    /* Необязательный эксперимент: меняем только путь к файлу.
       В обоих случаях вызываем один и тот же системный интерфейс execve(). */
    const char *path = "./build/05_exec/target_execve";
    if (argc == 2 && strcmp(argv[1], "--name-only") == 0) {
        path = "target_execve"; /* Относительный путь, НЕ поиск по PATH. */
    } else if (argc != 1) {
        fprintf(stderr, "usage: %s [--name-only]\n", argv[0]);
        return EXIT_FAILURE;
    }

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
        printf("launcher: child PID=%ld before execve\n", (long)getpid());
        printf("launcher: exec path=%s\n", path);
        fflush(stdout); /* exec заменит и буфер stdout старой программы. */

        /* Это argv будущей программы, НЕ аргументы самого launcher.
           argv[0] — имя по соглашению; NULL завершает массив. */
        char *const args[] = {"target_execve", "hello", "42", NULL};
        /* Полное новое окружение: не добавляем эти строки к старому.
           PARENT_ONLY специально отсутствует — target её не получит.
           PATH станет доступен target, но НЕ используется для поиска
           файла в данном вызове (путь указан первым аргументом). */
        char *const envp[] = {
            "LESSON_LABEL=from-envp",
            "PATH=./build/05_exec",
            NULL
        };
        execve(path, args, envp);

        /* Если exec успешен, этой строки никогда не достигнем:
           теперь в процессе с тем же PID выполняется target. */
        perror("execve");
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
