#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  /* Сравниваем три варианта семейства exec на одной задаче. */
  const char *mode = argc > 1 ? argv[1] : "execv";
  if (strcmp(mode, "execv") != 0 && strcmp(mode, "execvp") != 0 &&
      strcmp(mode, "execve") != 0) {
    fprintf(stderr, "usage: %s [execv|execvp|execve]\n", argv[0]);
    return EXIT_FAILURE;
  }

  /* Значение переменной сохраняется для execv/execvp.
     execve ниже задаёт окружение явно через отдельный массив envp. */
  if (setenv("LESSON_LABEL", "inherited environment", 1) == -1) {
    perror("setenv");
    return EXIT_FAILURE;
  }
  /* execvp ищет в PATH только имя файла без символа '/'.
     Относительные пути здесь предполагают запуск из корня проекта. */
  if (strcmp(mode, "execvp") == 0 &&
      setenv("PATH", "./build/05_exec:/usr/bin:/bin", 1) == -1) {
    perror("setenv PATH");
    return EXIT_FAILURE;
  }

  printf("launcher: parent PID=%ld mode=%s\n", (long)getpid(), mode);
  fflush(
      stdout); /* Исключаем дублирование невыведенных буферов после fork(). */

  /* Схема запуска оболочки: создаём ребёнка через fork(), в ребёнке
     загружаем другую программу через exec(), а родитель продолжает
     работать и ждёт результат выполнения потомка. */
  pid_t child = fork();
  if (child == -1) {
    perror("fork");
    return EXIT_FAILURE;
  }
  if (child == 0) {
    printf("launcher: child PID=%ld before exec\n", (long)getpid());
    fflush(stdout); /* Успешный exec заменит и буферы прежней программы. */

    /* argv[0] по соглашению содержит имя НОВОЙ программы;
       NULL обозначает конец массива аргументов. */
    char *const args[] = {"target", "hello", NULL};
    if (strcmp(mode, "execv") == 0) {
      execv("./build/05_exec/target",
            args); /* Программа задаётся явным путём. */
    } else if (strcmp(mode, "execvp") == 0) {
      execvp("target", args); /* Имя ищется по переменной PATH. */
    } else {
      char *const envp[] = {"LESSON_LABEL=explicit environment", NULL};
      execve("./build/05_exec/target", args,
             envp); /* Системный интерфейс с явным окружением. */
    }

    /* При успехе exec сюда не возвращается: начинает работать новая программа.
       Если управление вернулось, произошла ошибка; код 127 принят в
       оболочках как признак ошибки запуска, но не обязателен для ядра. */
    perror("exec");
    _exit(127);
  }

  /* Здесь всё ещё РОДИТЕЛЬ с прежним PID. Ребёнок сохранил свой PID,
     хотя его код, данные и стек теперь принадлежат новой программе. */
  int status = 0;
  if (waitpid(child, &status, 0) == -1) {
    perror("waitpid");
    return EXIT_FAILURE;
  }
  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    fprintf(stderr, "target failed (raw status=%d)\n", status);
    return EXIT_FAILURE;
  }
  printf("launcher: child %ld completed\n", (long)child);
  return EXIT_SUCCESS;
}
