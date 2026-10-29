#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Оба рабочих потока получат указатель на один объект из main().
   Здесь находятся и общий счётчик, и mutex для защиты счётчика. */
struct Shared {
  int counter;
  pthread_mutex_t mutex;
};

/* Каждому потоку — свои аргументы; оба указателя shared
   ссылаются при этом на один объект Shared. */
struct WorkerArgs {
  struct Shared *shared;
  int number;
};

/* Начальная функция pthread имеет сигнатуру void *(*)(void *).
   Два потока будут выполнять этот код конкурентно. */
void *worker(void *argument) {
  struct WorkerArgs *args = argument;
  printf("worker %d: PID=%ld TID=%ld shared-address=%p\n", args->number,
         (long)getpid(), (long)gettid(), (void *)&args->shared->counter);

  for (int i = 0; i < 1000; ++i) {
    /* counter++ — чтение, сложение и запись, а не атомарная операция.
       mutex не даёт возникнуть несинхронизированной гонке данных. */
    pthread_mutex_lock(&args->shared->mutex);
    args->shared->counter++;
    pthread_mutex_unlock(&args->shared->mutex);
  }
  return NULL; /* Завершаем только этот рабочий поток, не весь процесс. */
}

int main(void) {
  struct Shared shared = {.counter = 0};
  /* pthread_* обычно возвращают код ошибки напрямую и не обязаны
     записывать его в errno; поэтому применяем strerror(err). */
  int err = pthread_mutex_init(&shared.mutex, NULL);
  if (err != 0) {
    fprintf(stderr, "pthread_mutex_init: %s\n", strerror(err));
    return EXIT_FAILURE;
  }

  printf("main: PID=%ld TID=%ld shared-address=%p\n", (long)getpid(),
         (long)gettid(), (void *)&shared.counter);

  pthread_t threads[2];
  struct WorkerArgs args[2] = {{.shared = &shared, .number = 1},
                               {.shared = &shared, .number = 2}};

  int created = 0;
  for (int i = 0; i < 2; ++i) {
    /* Созданный поток может сразу начать worker(&args[i]); порядок
       вывода main и других рабочих потоков не определён. */
    err = pthread_create(&threads[i], NULL, worker, &args[i]);
    if (err != 0) {
      fprintf(stderr, "pthread_create: %s\n", strerror(err));
      break;
    }
    created++;
  }

  int success = created == 2;
  for (int i = 0; i < created; ++i) {
    /* Ждём до выхода из main: shared и args лежат на его стеке.
       Они обязаны существовать, пока рабочие потоки их используют. */
    int join_error = pthread_join(threads[i], NULL);
    if (join_error != 0) {
      fprintf(stderr, "pthread_join: %s\n", strerror(join_error));
      success = 0;
    }
  }
  printf("main: count=%d expected=%d\n", shared.counter, 2000);
  /* Уничтожаем mutex лишь после успешных join.
     При неожиданной ошибке join не удаляем потенциально занятый mutex. */
  if (success) {
    err = pthread_mutex_destroy(&shared.mutex);
    if (err != 0) {
      fprintf(stderr, "pthread_mutex_destroy: %s\n", strerror(err));
      success = 0;
    }
  }
  return success && shared.counter == 2000 ? EXIT_SUCCESS : EXIT_FAILURE;
}
