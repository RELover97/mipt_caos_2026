/*
 * Задача 1. Общий счётчик: data race и исправление через pthread_mutex.
 *
 * Запуск:
 *   ./build/01_counter unsafe  — намеренно некорректная программа (data race =>
 * UB)
 *   ./build/01_counter mutex   — корректная программа
 *
 * Четыре потока должны увеличить один общий счётчик по 100 000 раз.
 * Если бы все увеличения учитывались, результат был бы 400 000.
 *
 */

#include <inttypes.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { THREADS = 4, PER_THREAD = 100000 };

/* Глобальная переменная принадлежит процессу, поэтому доступна всем его
 * потокам. Она не атомарная: одновременно читать и записывать её без
 * синхронизации нельзя. */
uint64_t plain_counter = 0;

/* Изначально мьютекс разблокирован. Один мьютекс защищает один общий счётчик.
 * Инициализатор PTHREAD_MUTEX_INITIALIZER позволяет обойтись без init(). */
pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Выбирается в main до создания потоков и после этого никогда не меняется.
 * Поэтому работающие потоки только читают use_mutex. */
int use_mutex = 0;

/* Функция потока обязана иметь сигнатуру void *(*)(void *).
 * pthread_create начнёт выполнять её в новом потоке.
 * Аргумент здесь не используется, поэтому подавляем предупреждение компилятора.
 */
void *increment(void *unused) {
  (void)unused;

  for (int i = 0; i < PER_THREAD; ++i) {
    if (use_mutex) {
      /* Если другой поток уже захватил mutex, этот поток ждёт.
       * После успешного lock только он выполняет критическую секцию. */
      pthread_mutex_lock(&counter_mutex);

      /* Критическая секция: чтение, прибавление и запись защищены
       * одной блокировкой; другой поток не может вмешаться между ними. */
      ++plain_counter;

      /* Освобождаем mutex на каждой итерации. Иначе другие потоки
       * не смогут войти в критическую секцию. */
      pthread_mutex_unlock(&counter_mutex);
    } else {
      /* Data race (UB)
       * В двух потоках ++plain_counter конфликтует по одной памяти.
       * Возможная схема: A прочёл 5, B прочёл 5, оба записали 6. */
      ++plain_counter;
    }
  }

  /* NULL станет возвращённым значением pthread_join, если его запросить. */
  return NULL;
}

int main(int argc, char **argv) {
  /* argv[1] — выбранный пользователем сценарий. */
  if (argc != 2 ||
      (strcmp(argv[1], "unsafe") != 0 && strcmp(argv[1], "mutex") != 0)) {
    fprintf(stderr, "Usage: %s unsafe|mutex\n", argv[0]);
    return EXIT_FAILURE;
  }
  use_mutex = strcmp(argv[1], "mutex") == 0;

  /* pthread_t хранит идентификаторы потоков, чтобы позже их дождаться.
   * Массив находится в main, но управление потоками осуществляет pthread. */
  pthread_t threads[THREADS];
  for (int i = 0; i < THREADS; ++i) {
    /* NULL во втором аргументе — стандартные атрибуты потока.
     * increment — адрес функции потока; последний NULL — её аргумент. */
    if (pthread_create(&threads[i], NULL, increment, NULL) != 0) {
      fprintf(stderr, "pthread_create failed\n");
      return EXIT_FAILURE;
    }
  }

  /* join ждёт завершения каждого потока. Без join печать могла бы
   * произойти, пока потоки ещё увеличивают счётчик. */
  for (int i = 0; i < THREADS; ++i) {
    if (pthread_join(threads[i], NULL) != 0) {
      fprintf(stderr, "pthread_join failed\n");
      return EXIT_FAILURE;
    }
  }

  /* После join все потоки уже завершились: читать plain_counter безопасно
   * даже в mutex-режиме без дополнительного lock. */
  printf("mode=%s expected=%d observed=%" PRIu64 "\n", argv[1],
         THREADS * PER_THREAD, plain_counter);
  pthread_mutex_destroy(&counter_mutex);

  /* Для unsafe проверка результата не имеет смысла: там уже было UB.
   * Для mutex несоответствие 400 000 считаем ошибкой. */
  return use_mutex && plain_counter != (uint64_t)(THREADS * PER_THREAD)
             ? EXIT_FAILURE
             : EXIT_SUCCESS;
}
