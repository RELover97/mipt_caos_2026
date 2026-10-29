#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
    /* Первый дескриптор: обычный open, останется открытым после exec. */
    int plain = open("/dev/null", O_RDONLY);
    if (plain == -1) {
        perror("open plain");
        return EXIT_FAILURE;
    }

    /* O_CLOEXEC атомарно устанавливает флаг дескриптора FD_CLOEXEC.
       Он сохранится при fork(), но fd закроется при успешном exec. */
    int cloexec = open("/dev/null", O_RDONLY | O_CLOEXEC);
    if (cloexec == -1) {
        perror("open O_CLOEXEC");
        close(plain);
        return EXIT_FAILURE;
    }

    /* dup() указывает на то же открытое описание файла, но у НОВОГО
       дескриптора флаг FD_CLOEXEC будет снят; он переживёт exec. */
    int duplicated = dup(cloexec);
    if (duplicated == -1) {
        perror("dup");
        close(plain);
        close(cloexec);
        return EXIT_FAILURE;
    }

    /* execv принимает строковые аргументы: переводим номера в строки.
       Затем fd_probe.c обратно преобразует строки через strtol. */
    char plain_text[32], cloexec_text[32], duplicated_text[32];
    snprintf(plain_text, sizeof(plain_text), "%d", plain);
    snprintf(cloexec_text, sizeof(cloexec_text), "%d", cloexec);
    snprintf(duplicated_text, sizeof(duplicated_text), "%d", duplicated);
    printf("parent before fork: plain=%d cloexec=%d dup=%d\n",
           plain, cloexec, duplicated);
    fflush(stdout);

    /* fork() копирует записи таблицы fd; затем exec() закрывает только
       те дескрипторы, для которых установлен флаг FD_CLOEXEC. */
    pid_t child = fork();
    if (child == -1) {
        perror("fork");
        close(plain);
        close(cloexec);
        close(duplicated);
        return EXIT_FAILURE;
    }
    if (child == 0) {
        char *const args[] = { "fd_probe", plain_text, cloexec_text,
                               duplicated_text, NULL };
        execv("./build/08_fds/fd_probe", args);
        perror("execv fd_probe"); /* Только при ошибке execv управление вернётся сюда. */
        _exit(127);
    }

    int status = 0;
    pid_t reaped = waitpid(child, &status, 0);
    if (reaped == -1) {
        perror("waitpid");
    }
    /* У родителя собственная таблица fd: он тоже закрывает свои номера. */
    close(plain);
    close(cloexec);
    close(duplicated);
    if (reaped == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
