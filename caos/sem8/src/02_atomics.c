/*
 * Задача 2. Атомарные операции: data race можно убрать, но логическую гонку —
 * нет.
 *
 * Режимы: split  — ошибка при двух отдельных atomic load/store;
 *          fetch — корректное увеличение одним fetch_add;
 *          cas   — увеличение с помощью compare-and-swap в цикле.
 *
 * Все обращения к общему counter атомарные — data race в этих режимах нет.
 */
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* atomic_int — атомарный целочисленный объект стандарта C11.
 * Он общий для потоков, но его изменение происходит атомарными операциями. */
atomic_int counter = ATOMIC_VAR_INIT(0);

/* Барьер исключительно учебный: он не даёт двум потокам продолжить
 * выполнение, пока оба не дойдут до pthread_barrier_wait().
 * Так мы управляем порядком операций, не надеясь на случайный планировщик. */
pthread_barrier_t both_loaded;

enum { FETCH_THREADS = 4, FETCH_ITERATIONS = 100000 };

/* Правильное решение для счётчика: у C11 уже есть неделимый fetch_add.
 * relaxed достаточно, потому что counter ничего не сообщает о других данных. */
void *fetch_worker(void *unused) {
  (void)unused;
  for (int i = 0; i < FETCH_ITERATIONS; ++i) {
    /* CPU/реализация атомарно читает текущее число, прибавляет 1
     * и записывает новое. Другой fetch_add не «потеряется» между ними. */
    atomic_fetch_add_explicit(&counter, 1, memory_order_relaxed);
  }
  return NULL;
}

/* Намеренная логическая ошибка: отдельные атомарные действия
 * не образуют одну атомарную транзакцию. */
void *split_worker(void *unused) {
  (void)unused;

  /* Изначально counter = 0. Оба потока сохраняют локальный снимок 0.
   * snapshot — обычная переменная в стеке каждого потока, она не общая. */
  int snapshot = atomic_load_explicit(&counter, memory_order_relaxed);

  /* Ни один поток не сможет перейти к store, пока второй не выполнит load.
   * Следовательно, оба позднее попытаются записать 0 + 1. */
  pthread_barrier_wait(&both_loaded);

  /* Каждая запись атомарна, но два потока по очереди записывают один и тот
   * же результат 1. Желаемые два прибавления превращаются в одно. */
  atomic_store_explicit(&counter, snapshot + 1, memory_order_relaxed);
  return NULL;
}

/* CAS = compare-and-swap: «замени значение, только если оно не изменилось».
 * Эта функция демонстрирует, как исправить split без mutex. */
void *cas_worker(void *unused) {
  (void)unused;

  /* expected — локальная копия того, что мы предполагаем увидеть
   * в общем counter на следующем CAS. */
  int expected = atomic_load_explicit(&counter, memory_order_relaxed);
  pthread_barrier_wait(&both_loaded);

  /* CAS на успехе делает counter = expected + 1 (неделимо с проверкой).
   * На неудаче ничего не записывает в counter, но переписывает expected
   * реальным значением counter. Поэтому следующая попытка использует
   * актуальное число, а не старый снимок.
   *
   * weak-CAS также может иногда вернуть false без изменения counter;
   * цикл повторяет попытку и в этом случае. */
  while (!atomic_compare_exchange_weak_explicit(
      &counter, &expected, expected + 1, memory_order_relaxed,
      memory_order_relaxed)) {
    /* Тело пустое: вся работа происходит в условии while.
     * При сильной конкуренции этот цикл может потребовать несколько попыток. */
  }
  return NULL;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s split|cas|fetch\n", argv[0]);
    return EXIT_FAILURE;
  }

  int split = strcmp(argv[1], "split") == 0;
  int cas = strcmp(argv[1], "cas") == 0;
  int fetch = strcmp(argv[1], "fetch") == 0;

  if (!split && !cas && !fetch) {
    fprintf(stderr, "Unknown mode: %s\n", argv[1]);
    return EXIT_FAILURE;
  }

  /* split и cas используют два потока и барьер, fetch — четыре потока.
   * worker — указатель на одну из трёх функций потока (одинаковая сигнатура).
   */
  int n = fetch ? FETCH_THREADS : 2;

  void *(*worker)(void *) =
      fetch ? fetch_worker : (split ? split_worker : cas_worker);

  /* Барьер будет пропускать потоки партиями по n=2.
   * Для fetch он не нужен: хотим свободную конкуренцию за counter. */
  if (!fetch && pthread_barrier_init(&both_loaded, NULL, (unsigned)n) != 0) {
    fprintf(stderr, "pthread_barrier_init failed\n");
    return EXIT_FAILURE;
  }

  pthread_t threads[FETCH_THREADS];
  for (int i = 0; i < n; ++i) {
    if (pthread_create(&threads[i], NULL, worker, NULL) != 0) {
      fprintf(stderr, "pthread_create failed\n");
      return EXIT_FAILURE;
    }
  }

  for (int i = 0; i < n; ++i) {
    pthread_join(threads[i], NULL);
  }

  if (!fetch) {
    /* Барьер можно уничтожать лишь после завершения всех его участников. */
    pthread_barrier_destroy(&both_loaded);
  }

  int result = atomic_load_explicit(&counter, memory_order_relaxed);
  /* Для split ожидаем воспроизводимый неверный итог 1,
   * для cas — верный итог 2, для fetch — 400 000. */
  int expected = fetch ? FETCH_THREADS * FETCH_ITERATIONS : (split ? 1 : 2);

  printf("mode=%s value=%d (expected in this scenario=%d)\n", argv[1], result,
         expected);

  return result == expected ? EXIT_SUCCESS : EXIT_FAILURE;
}
