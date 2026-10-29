#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Это отдельый исполняемый файл, а не функция из launcher.c.
   Ребёнок launcher заменяет себя этой программой при успешном exec. */
int main(int argc, char *argv[]) {
  /* Сравните PID со строкой launcher "child PID ... before exec".
     Они равны: exec не создаёт нового процесса. */
  printf("target: PID=%ld PPID=%ld\n", (long)getpid(), (long)getppid());
  printf("target: argument=%s\n", argc > 1 ? argv[1] : "(none)");

  /* getenv читает окружение новой программы: унаследованное через
     execv/execvp или явно переданное массивом envp при execve. */
  const char *label = getenv("LESSON_LABEL");
  printf("target: LESSON_LABEL=%s\n", label == NULL ? "(unset)" : label);
  return EXIT_SUCCESS;
}
