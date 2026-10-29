#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/* Функция работает в новом образе программы, загруженном через exec.
   Локальные переменные родительского кода недоступны: номера fd
   переданы в argv как строки. */
void inspect_fd(const char *label, const char *number) {
  char *end = NULL;
  errno = 0;
  long descriptor = strtol(number, &end, 10);
  if (errno != 0 || end == number || *end != '\0' || descriptor < 0 ||
      descriptor > INT_MAX) {
    fprintf(stderr, "invalid descriptor: %s\n", number);
    exit(EXIT_FAILURE);
  }

  /* F_GETFD читает флаг дескриптора FD_CLOEXEC, а не режим открытого файла.
     Ошибка EBADF означает, что этот номер fd не открыт. */
  errno = 0;
  int flags = fcntl((int)descriptor, F_GETFD);
  if (flags == -1 && errno == EBADF) {
    printf("%s: closed\n", label);
  } else if (flags >= 0) {
    printf("%s: open\n", label);
  } else {
    perror("fcntl");
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    fprintf(stderr, "usage: %s <plain-fd> <cloexec-fd> <dup-fd>\n", argv[0]);
    return EXIT_FAILURE;
  }
  /* fd_parent уже выполнил execv: проверяем состояние после замены. */
  inspect_fd("plain", argv[1]);
  inspect_fd("cloexec", argv[2]);
  inspect_fd("dup-of-cloexec", argv[3]);
  return EXIT_SUCCESS;
}
