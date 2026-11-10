/*
 * ЗАДАЧА 4. Futex — низкоуровневое «сравнить значение и уснуть» в Linux.
 *
 * ./build/04_futex normal  — ожидающий поток ждёт ready=1;
 * ./build/04_futex late    — ready=1 выставлен ДО FUTEX_WAIT;
 *                            ядро возвращает EAGAIN, не усыпляя поток.
 *
 * Пример ориентирован на Linux и поддерживаемые GCC/ABI; futex — НЕ
 * переносимая POSIX-замена pthread_mutex/condition variable.
 */
#include <errno.h>
#include <linux/futex.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

/* Системный вызов futex ожидает адрес корректно выровненного слова на 32 бита.
 * _Atomic uint32_t обеспечивает атомарный доступ из пользовательского кода.
 * Следующие проверки гарантируют размер/выравнивание, но не являются
 * универсальным доказательством совместимости всех ABI. */
_Static_assert(sizeof(_Atomic uint32_t) == sizeof(uint32_t), "futex word must be 32 bits");
_Static_assert(_Alignof(_Atomic uint32_t) >= 4, "futex word must be aligned");

/* ready = 0 — данные ещё не готовы; ready = 1 — можно продолжать.
 * Главное: само СОСТОЯНИЕ хранится в памяти и не теряется, в отличие от
 * уведомления, отправленного до появления ожидающих потоков. */
_Atomic uint32_t ready = ATOMIC_VAR_INIT(0);

/* Тонкая обёртка над системным вызовом Linux.
 * FUTEX_WAIT_PRIVATE просит ядро: «усни ТОЛЬКО ЕСЛИ *word == expected».
 * Проверка и постановка в очередь ожидания происходят атомарно друг с другом.
 * Если значение уже другое, syscall возвращает -1, errno = EAGAIN. */
int futex_wait(_Atomic uint32_t *word, uint32_t expected) {
    return (int)syscall(SYS_futex, (uint32_t *)word,
                        FUTEX_WAIT_PRIVATE, expected, NULL, NULL, 0);
}

/* FUTEX_WAKE_PRIVATE будит максимум ОДИН поток, который ждёт на word.
 * Возвращённое число — сколько ожидающих потоков действительно разбужено.
 * Функция НЕ присваивает слову новое значение! */
int futex_wake(_Atomic uint32_t *word) {
    return (int)syscall(SYS_futex, (uint32_t *)word,
                        FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);
}

void *waiter(void *unused) {
    (void)unused;

    /* Сначала смотрим на атомарное состояние в пользовательском пространстве.
     * Если ready уже 1, обращаться к ядру вообще не нужно. */
    while (atomic_load_explicit(&ready, memory_order_acquire) == 0) {
        /* Пока флаг равен 0, можно попробовать заснуть.
         * Между load выше и futex_wait главный поток МОЖЕТ записать 1.
         * Это безопасно: ядро увидит несовпадение с expected=0
         * и вместо сна вернёт EAGAIN. */
        int rc = futex_wait(&ready, 0);

        /* EAGAIN — состояние поменялось до сна, это обычный исход.
         * EINTR — ожидание было прервано (например, сигналом).
         * Даже после rc==0 (пробуждения) состояние нужно проверить заново:
         * FUTEX_WAKE — не обещание того, что ready==1. */
        if (rc == -1 && errno != EAGAIN && errno != EINTR) {
            perror("futex FUTEX_WAIT_PRIVATE");
            return (void *)1;
        }
    }

    puts("waiter: observed ready=1, proceeding");
    return NULL;
}

/* Искусственная задержка делает normal-режим нагляднее, но НЕ гарантирует,
 * что waiter успел заснуть: на загруженной системе wake может вернуть 0. */
void pause_ms(long ms) {
    struct timespec ts = {.tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000L};
    nanosleep(&ts, NULL);
}

int main(int argc, char **argv) {
    if (argc != 2 || (strcmp(argv[1], "normal") != 0 && strcmp(argv[1], "late") != 0)) {
        fprintf(stderr, "Usage: %s normal|late\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "late") == 0) {
        /* Полностью детерминированный контрпример в одном потоке.
         * Сначала записываем ready=1, затем пытаемся «поздно» заснуть.
         * Нет ни одного ожидающего потока, поэтому wake должен вернуть 0. */
        atomic_store_explicit(&ready, 1, memory_order_release);
        int woken = futex_wake(&ready);

        errno = 0; /* Сбрасываем старое значение для проверки именно wait. */
        int rc = futex_wait(&ready, 0);
        int saved_errno = errno; /* Сохраняем СРАЗУ после syscall. */

        /* Ядро сравнило 1 != 0 и не усыпило нас; ожидаем EAGAIN.
         * В отличие от pthread_cond_t, «состояние уже изменилось»
         * не может потеряться между проверкой и постановкой на сон. */
        printf("late: woken=%d wait=%d errno=%d (EAGAIN=%d)\n",
               woken, rc, saved_errno, EAGAIN);
        return (woken == 0 && rc == -1 && saved_errno == EAGAIN)
                   ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    /* normal: отдельный поток выполняет waiter(), main меняет ready. */
    pthread_t thread;
    if (pthread_create(&thread, NULL, waiter, NULL) != 0) {
        fprintf(stderr, "pthread_create failed\n");
        return EXIT_FAILURE;
    }
    pause_ms(20);

    /* СНАЧАЛА публикуем состояние ready=1, ПОТОМ будим ожидающих.
     * release на записи + acquire при чтении формируют пару публикации:
     * если бы перед ready мы подготовили другие данные, waiter смог бы
     * увидеть их после acquire, увидев 1. */
    atomic_store_explicit(&ready, 1, memory_order_release);
    int woken = futex_wake(&ready);
    if (woken == -1) {
        perror("futex FUTEX_WAKE_PRIVATE");
        return EXIT_FAILURE;
    }

    /* 0 тоже допустимо: waiter мог ещё не заснуть или уже заметил ready=1.
     * Поэтому не стоит воспринимать wake-count как индикатор успешности. */
    printf("main: wake count=%d (0 or 1 are both valid)\n", woken);

    void *result = NULL;
    pthread_join(thread, &result);
    return result == NULL ? EXIT_SUCCESS : EXIT_FAILURE;
}
