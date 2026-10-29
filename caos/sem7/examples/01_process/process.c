#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Объекты со статическим временем жизни обычно размещаются в .data/.bss. */
int global_initialized = 7;  /* Ненулевая инициализация: обычно секция .data. */
int global_zero_initialized; /* Нулевая инициализация: обычно секция .bss. */

int main(void) {
  int local = 11; /* Локальная переменная: обычно стек основного потока. */

  /* malloc выделяет память в виртуальном адресном пространстве процесса.
     Внутри могут использоваться brk или mmap; системный вызов не обязан
     выполняться при каждом malloc. Указатель принадлежит этому процессу. */
  int *heap = malloc(sizeof(*heap));
  if (heap == NULL) {
    perror("malloc");
    return EXIT_FAILURE;
  }
  *heap = 22;

  /* getpid() возвращает PID группы потоков, а gettid() — TID задачи ядра.
     У однопоточного процесса эти идентификаторы обычно совпадают. */
  printf("PID=%ld PPID=%ld TID=%ld\n", (long)getpid(), (long)getppid(),
         (long)gettid());

  /* Напечатаны виртуальные адреса, а не адреса физической памяти.
     Из-за ASLR между запусками они часто различаются. */
  printf("addresses: data=%p bss=%p stack=%p heap=%p\n",
         (void *)&global_initialized, (void *)&global_zero_initialized,
         (void *)&local, (void *)heap);

  /* procfs предоставляет сведения ядра о текущем процессе.
     /proc/self относится к процессу, вызывающему fopen().
     Файл не является копией структуры task_struct в пользовательской памяти. */
  FILE *status = fopen("/proc/self/status", "r");
  if (status == NULL) {
    perror("/proc/self/status");
    free(heap);
    return EXIT_FAILURE;
  }

  /* Выведем только поля, нужные для первого этапа.
     Строки создаются ядром, а не читаются из обычного дискового файла. */
  char line[256];
  const char *keys[] = {"Name:", "State:",   "Tgid:",   "Pid:",
                        "PPid:", "Threads:", "VmSize:", "VmRSS:"};
  while (fgets(line, sizeof(line), status) != NULL) {
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i) {
      if (strncmp(line, keys[i], strlen(keys[i])) == 0) {
        fputs(line, stdout);
        break;
      }
    }
  }
  if (ferror(status)) {
    perror("fgets");
    fclose(status);
    free(heap);
    return EXIT_FAILURE;
  }

  fclose(status); /* Закрываем файл и принадлежащий FILE файловый дескриптор. */
  free(heap);     /* Освобождаем динамически выделенную память процесса. */
  return EXIT_SUCCESS;
}
