/*
 * Задача 3. Condition variable и ожидание изменения состояния.
 *
 * ./build/03_condition good — производитель/потребитель, корректный протокол
 * ./build/03_condition lost — контролируемая ошибка missing wakeup
 *
 * pthread_cond_t не является очередью сигналов. Программа должна хранить
 * условие (предикат: full/closed/ready), проверять его под одним mutex
 * и повторять проверку после каждого возвращения из pthread_cond_wait.
 */

#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Очередь из одной ячейки.
 * Когда full=false, производитель вправе записать value.
 * Когда full=true, потребитель вправе прочитать value.
 * Все поля структуры, включая sum и empty_wakeups, защищаются mutex.
 * Нельзя смотреть на full без mutex одновременно с работой потоков! */
typedef struct {
  pthread_mutex_t mutex;    /* Единственная блокировка для состояния ячейки. */
  pthread_cond_t not_empty; /* Здесь ждёт потребитель: full или closed. */
  pthread_cond_t not_full;  /* Здесь ждёт производитель: !full. */
  int value;                /* Данные в ячейке, осмысленны при full=true. */
  bool full;                /* Предикат «ячейка занята». */
  bool closed;              /* Новых значений после завершения не будет. */
  unsigned empty_wakeups;   /* Сколько раз просыпались, но данных не было. */
  int sum;                  /* Сумма обработанных значений. */
} Slot;

/* Объект с глобальным временем жизни; неуказанные поля равны нулю.
 * Поэтому full=false, closed=false, sum=0 при запуске. */
Slot slot = {.mutex = PTHREAD_MUTEX_INITIALIZER,
             .not_empty = PTHREAD_COND_INITIALIZER,
             .not_full = PTHREAD_COND_INITIALIZER};

enum { ITEMS = 10 };

/* Небольшая пауза только для наглядности чередования потоков.
 * nanosleep не является средством синхронизации: он не гарантирует порядок. */
void pause_ms(long ms) {
  struct timespec ts = {.tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000L};
  nanosleep(&ts, NULL);
}

/* Производитель должен передать последовательно 1, 2, ..., ITEMS. */
void *producer(void *unused) {
  (void)unused;
  for (int value = 1; value <= ITEMS; ++value) {
    pause_ms(2); /* Даём потребителю и noise время попасть в ожидание. */

    /* Захватываем mutex перед проверкой full. */
    pthread_mutex_lock(&slot.mutex);

    /* Пока ячейка занята, записывать нельзя: потеряли бы старое значение.
     * wait атомарно относительно протокола condition variable:
     *   1) отпускает slot.mutex, чтобы потребитель мог забрать число;
     *   2) блокирует поток до пробуждения;
     *   3) перед возвратом снова захватывает slot.mutex.
     * Поэтому условие проверяется снова в while, а не один раз через if. */
    while (slot.full) {
      pthread_cond_wait(&slot.not_full, &slot.mutex);
    }

    /* Здесь ячейка пуста, и mutex всё ещё наш. */
    slot.value = value;

    slot.full = true; /* Сначала меняем само условие ожидания */

    pthread_cond_signal(&slot.not_empty); /* затем будим потребителя. */
    pthread_mutex_unlock(&slot.mutex);    /* Даём потребителю войти. */
  }

  /* После отправки последнего элемента сообщаем: новых больше не будет.
   * Потребитель должен закончить работу только после обработки full-ячейки.
   * Поэтому его условие выхода — одновременно closed && !full. */
  pthread_mutex_lock(&slot.mutex);

  slot.closed = true;

  pthread_cond_broadcast(&slot.not_empty);
  pthread_mutex_unlock(&slot.mutex);

  return NULL;
}

/* Потребитель забирает числа, пока очередь не опустела и не закрыта. */
void *consumer(void *unused) {
  (void)unused;
  for (;;) {
    pthread_mutex_lock(&slot.mutex);

    /* Ждём, только если нет данных и производитель ещё может их прислать.
     * Любое пробуждение — повод заново проверить этот предикат. */
    while (!slot.full && !slot.closed) {
      pthread_cond_wait(&slot.not_empty, &slot.mutex);

      /* Если проснулись, но full=false и closed=false, уведомление
       * не означает «элемент появился». В good-режиме это может
       * вызывать поток noise; возможны также spurious wakeups POSIX. */
      if (!slot.full && !slot.closed) {
        ++slot.empty_wakeups;
      }
    }

    /* Ячейка пуста, а производитель завершил отправку — выходим.
     * Перед break обязательно освободить mutex! */
    if (!slot.full && slot.closed) {
      pthread_mutex_unlock(&slot.mutex);
      break;
    }

    /* Здесь full=true, иначе мы бы ещё ждали или вышли.
     * Обновляем сумму, помечаем ячейку свободной и будим производителя. */
    slot.sum += slot.value;

    slot.full = false;

    pthread_cond_signal(&slot.not_full);
    pthread_mutex_unlock(&slot.mutex);
  }
  return NULL;
}

/* Специальный третий поток посылает неполезные уведомления.
 * Важно: это не «истинные» spurious wakeups самой реализации pthread:
 * signal здесь вызывается явно, но без изменения full/closed.
 * Эффект тот же: после пробуждения предикат может оставаться ложным. */
void *noise(void *unused) {
  (void)unused;
  for (int i = 0; i < 30; ++i) {
    pause_ms(1);
    pthread_mutex_lock(&slot.mutex);
    pthread_cond_broadcast(&slot.not_empty);
    pthread_mutex_unlock(&slot.mutex);
  }
  return NULL;
}

/* Второй, независимый сценарий — missing wakeup.
 * ready специально атомарный: на нём нет data race.
 * Логическая ошибка состоит в том, что bad_waiter решает ждать по старой
 * проверке ready, сделанной без того mutex, который передаётся cond_wait. */
atomic_bool ready = ATOMIC_VAR_INIT(false);
pthread_mutex_t lost_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t lost_cond = PTHREAD_COND_INITIALIZER;

/* Барьеры обеспечивают порядок воспроизведения ошибки:
 * checked  — ждущий поток прочитал ready=false;
 * notified — отправитель УЖЕ сделал signal до попытки ожидания.
 * Без барьеров порядок определялся бы случайным планировщиком. */
pthread_barrier_t checked;
pthread_barrier_t notified;

void *bad_waiter(void *unused) {
  (void)unused;

  /* Шаг 1. Проверили ready, пока значение false.
   * atomic_load предотвращает data race, но не делает решение ждать верным. */
  bool was_ready = atomic_load_explicit(&ready, memory_order_acquire);

  pthread_barrier_wait(&checked);  /* Отправителю разрешено продолжить. */
  pthread_barrier_wait(&notified); /* Ждём, пока он отправит уведомление. */

  /* Шаг 3. Ошибка: was_ready — устаревший снимок.
   * Из-за барьера уведомление точно было раньше timedwait.
   * Condition variable не запоминает уведомления для будущих wait. */
  int rc = 0;
  if (!was_ready) {
    pthread_mutex_lock(&lost_mutex);

    /* Вместо бесконечного cond_wait используем timedwait: плохой код
     * должен показать проблему, а не зависнуть навсегда.
     * POSIX ждёт АБСОЛЮТНОГО момента CLOCK_REALTIME, не «100 мс» сам. */
    struct timespec deadline;
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_nsec += 100000000L; /* Добавляем 100 миллисекунд. */
    if (deadline.tv_nsec >= 1000000000L) {
      ++deadline.tv_sec;               /* Переносим полную секунду... */
      deadline.tv_nsec -= 1000000000L; /* ...и нормализуем наносекунды. */
    }

    /* Отпускаем mutex и ожидаем уведомления, которое уже произошло.
     * На нормальном ходе демонстрации возвращается ETIMEDOUT. */
    rc = pthread_cond_timedwait(&lost_cond, &lost_mutex, &deadline);
    pthread_mutex_unlock(&lost_mutex);
  }

  printf("missing-wakeup: ready_now=%d, wait_result=%s\n",
         (int)atomic_load_explicit(&ready, memory_order_acquire),
         rc == ETIMEDOUT ? "ETIMEDOUT (notification lost)"
                         : "unexpected wakeup");

  /* join получит NULL в ожидаемом сценарии; (void *)1 — ошибку.
   * В этом учебном примере мы используем возвращённый указатель как флаг. */
  return (void *)(long)(rc == ETIMEDOUT ? 0 : 1);
}

void *bad_notifier(void *unused) {
  (void)unused;
  pthread_barrier_wait(&checked); /* Ждём, пока waiter прочитает false. */

  /* Шаг 2. Обновляем состояние и посылаем сигнал до вызова wait.
   * lost_mutex сам по себе не исправляет ошибку, потому что waiter
   * проверил ready раньше, не удерживая lost_mutex. */
  pthread_mutex_lock(&lost_mutex);

  atomic_store_explicit(&ready, true, memory_order_release);
  pthread_cond_signal(&lost_cond);

  pthread_mutex_unlock(&lost_mutex);

  pthread_barrier_wait(&notified); /* Теперь разрешаем waiter заснуть. */

  return NULL;
}

int run_good(void) {
  pthread_t p, c, n;

  /* Порядок создания НЕ гарантирует порядок реального исполнения.
   * Корректная очередь работает при любом допустимом расписании. */
  if (pthread_create(&c, NULL, consumer, NULL) ||
      pthread_create(&p, NULL, producer, NULL) ||
      pthread_create(&n, NULL, noise, NULL)) {
    fprintf(stderr, "pthread_create failed\n");
    return EXIT_FAILURE;
  }

  pthread_join(p, NULL);
  pthread_join(c, NULL);
  pthread_join(n, NULL);

  /* Сумма первых 10 натуральных чисел: 10 * 11 / 2 = 55.
   * empty_wakeups может быть даже 0: сколько уведомлений попадут
   * на реально ожидающий поток, зависит от расписания. */
  printf("queue: sum=%d expected=55, wakeups_with_false_predicate=%u\n",
         slot.sum, slot.empty_wakeups);
  int result = slot.sum == 55 ? EXIT_SUCCESS : EXIT_FAILURE;

  /* Условные переменные и mutex уничтожаем ТОЛЬКО после pthread_join:
   * теперь никто не ждёт на not_empty/not_full и не держит mutex.
   * Для динамически создаваемых объектов есть pthread_cond_init и
   * pthread_mutex_init; здесь использованы PTHREAD_*_INITIALIZER. */
  pthread_cond_destroy(&slot.not_empty);
  pthread_cond_destroy(&slot.not_full);
  pthread_mutex_destroy(&slot.mutex);
  return result;
}

int run_lost(void) {
  /* Для двух потоков: первый прибывший ждёт второго, затем оба идут дальше. */
  pthread_barrier_init(&checked, NULL, 2);
  pthread_barrier_init(&notified, NULL, 2);
  pthread_t waiter, notifier;
  if (pthread_create(&waiter, NULL, bad_waiter, NULL) ||
      pthread_create(&notifier, NULL, bad_notifier, NULL)) {
    fprintf(stderr, "pthread_create failed\n");
    return EXIT_FAILURE;
  }

  void *result = NULL;
  pthread_join(waiter, &result);
  pthread_join(notifier, NULL);
  pthread_barrier_destroy(&checked);
  pthread_barrier_destroy(&notified);
  /* Демонстрация завершена: никто больше не ждёт, уничтожаем примитивы. */
  pthread_cond_destroy(&lost_cond);
  pthread_mutex_destroy(&lost_mutex);

  /* Тест считается успешным, только если воспроизвели ожидаемую ошибку. */
  return result == NULL ? EXIT_SUCCESS : EXIT_FAILURE;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s good|lost\n", argv[0]);
    return EXIT_FAILURE;
  }
  if (strcmp(argv[1], "good") == 0) {
    return run_good();
  }
  if (strcmp(argv[1], "lost") == 0) {
    return run_lost();
  }
  fprintf(stderr, "Unknown mode\n");
  return EXIT_FAILURE;
}
