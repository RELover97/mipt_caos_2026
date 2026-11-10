/*
 * Знакомство с потоками: создание, аргументы, общая память, pthread_join.
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

/* Глобальная переменная — одна на весь процесс, общая для всех его потоков. */
int shared_number = 42;

enum { THREAD_COUNT = 2 };

/* pthread_create требует функцию с параметром void* и результатом void*.
 * Мы передадим адрес отдельного int для каждого потока и приведём его тип. */
void *thread_work(void *argument) {
  int id = *(int *)argument;

  /* local_counter лежит в стеке именно этого потока. У второго потока
   * будет другой local_counter, даже если код функции один и тот же. */
  int local_counter = 0;
  for (int step = 0; step < 3; ++step) {
    ++local_counter;
    /* printf может выполняться в любом порядке: выбор момента запуска
     * потоков принадлежит планировщику ОС. Сама библиотечная printf
     * синхронизирует работу с потоком вывода. */
    printf("thread=%d step=%d local=%d shared=%d\n", id, step + 1,
           local_counter, shared_number);
  }

  /* Завершить функцию потока: вызывающий main затем дождётся join(). */
  return NULL;
}

int main(void) {
  pthread_t threads[THREAD_COUNT]; /* Идентификаторы созданных потоков. */
  int ids[THREAD_COUNT] = {1, 2};  /* Разные, живые до join аргументы. */

  for (int i = 0; i < THREAD_COUNT; ++i) {
    /* &threads[i] — куда сохранить дескриптор нового потока.
     * NULL — атрибуты по умолчанию.
     * thread_work — функция, которую начнёт исполнять поток.
     * &ids[i] — её аргумент, адрес своего id.
     * После успешного вызова main и новый поток работают конкурентно. */
    if (pthread_create(&threads[i], NULL, thread_work, &ids[i]) != 0) {
      fprintf(stderr, "pthread_create failed\n");
      return EXIT_FAILURE;
    }
  }

  for (int i = 0; i < THREAD_COUNT; ++i) {
    /* join ждёт завершения конкретного потока.
     * Благодаря join массив ids не исчезнет раньше завершения работы. */
    if (pthread_join(threads[i], NULL) != 0) {
      fprintf(stderr, "pthread_join failed\n");
      return EXIT_FAILURE;
    }
  }

  /* В этот момент ни один worker уже не работает. */
  puts("main: both threads joined; shared_number=42");
  return EXIT_SUCCESS;
}
