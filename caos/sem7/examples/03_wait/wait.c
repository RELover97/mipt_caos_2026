#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
  /* При fork() ребёнок наследует ещё не выведенные буферы stdio. */
  fflush(stdout);

  pid_t child = fork();
  if (child < 0) {
    perror("fork");
    return EXIT_FAILURE;
  }
  if (child == 0) {
    /* exit() сбрасывает буферы stdio и выполняет обычную очистку C.
       Число 23 — код завершения, а не необработанное слово status из wait(). */
    puts("child: exit(23) ends this process");
    exit(23);
  }

  /* Родитель продолжает исполняться. Без wait/waitpid завершившийся
     ребёнок может временно остаться zombie: он уже не работает, но
     его статус ещё не собран. Здесь у нас ровно один ребёнок. */
  int status = 0;
  pid_t reaped = wait(&status); /* Блокируемся, пока не завершится и не будет
                                   собран один из детей. */
  if (reaped == -1) {
    perror("wait");
    return EXIT_FAILURE;
  }
  if (reaped != child) {
    fputs("unexpected child PID\n", stderr);
    return EXIT_FAILURE;
  }

  /* status кодирован: это не просто число, переданное в exit().
     WEXITSTATUS допустим только после успешной проверки WIFEXITED. */
  if (WIFEXITED(status)) {
    printf("parent: reaped=%ld exit=%d\n", (long)reaped, WEXITSTATUS(status));
  } else {
    fputs("parent: child did not exit normally\n", stderr);
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
