#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
  /* Проверим, изменится ли переменная родителя, если записать в неё
     другое значение в только что созданном дочернем процессе. */
  int value = 10;
  printf("before fork: PID=%ld value=%d\n", (long)getpid(), value);
  fflush(stdout);
  /* stdout может буферизовать вывод (особенно при перенаправлении).
     Сбрасываем буфер до fork(), чтобы ребёнок не напечатал эти байты дважды. */

  /* До fork() выполнялся один процесс. После вызова оба процесса
     продолжают со следующей строки, но получают разные значения. */
  pid_t child = fork();
  if (child == -1) {
    perror("fork"); /* Ошибка: создать дочерний процесс не удалось. */
    return EXIT_FAILURE;
  }

  if (child == 0) {
    /* дочерний процесс: fork() вернул 0, но настоящий PID ребёнка ненулевой.
       Начальное значение переменной было скопировано логически;
       запись не меняет память родителя благодаря copy-on-write. */
    value = 99;
    printf("child: PID=%ld PPID=%ld value=%d\n", (long)getpid(),
           (long)getppid(), value);
    fflush(stdout);
    /* _exit() не выполняет обычную очистку stdio.
       Поэтому печать перед ним обязательно сбрасываем через fflush(). */
    _exit(EXIT_SUCCESS);
  }

  /* родитель: child содержит PID нового процесса (> 0).
     waitpid ждёт именно его и забирает результат завершения.
     Поэтому последняя строка появится после сообщения ребёнка. */
  if (waitpid(child, NULL, 0) == -1) {
    perror("waitpid");
    return EXIT_FAILURE;
  }
  printf("parent: PID=%ld child=%ld value=%d\n", (long)getpid(), (long)child,
         value);
  return EXIT_SUCCESS;
}
