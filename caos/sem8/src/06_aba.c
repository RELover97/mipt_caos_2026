/*
 * ЗАДАЧА 6. ABA — почему даже успешный CAS не всегда означает правильный pop.
 *
 * ./build/06_aba
 *
 * Сначала стек A -> B -> C. Поток 1 запоминает head=A и next=B.
 * Затем поток 2 снимает A, снимает B, заново кладёт A: стек A -> C.
 * Поток 1 пытается CAS(head, A, B), и он УСПЕШЕН, потому что head снова A!
 * Узел B уже был извлечён, но теперь «воскресает» в цепочке. Узел A теряется.
 *
 * В демонстрации узлы НЕ освобождаются, поэтому мы изолируем именно
 * логическую проблему ABA, а не use-after-free. Это НАМЕРЕННО плохой pop.
 */
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* Упрощённый узел; id нужен только для понятной печати A/B/C. */
typedef struct Node {
    char id;
    struct Node *next;
} Node;

/* Три реальных объекта с неизменными адресами на всё время программы. */
Node a, b, c;
_Atomic(Node *) top; /* Общий атомарный указатель на вершину. */

/* Барьеры используются как «режиссёр» расписания двух потоков.
 * snapshot_taken: T1 сохранил A и B; T2 может менять стек.
 * changed_back: T2 восстановил head=A, но цепочка уже другая. */
pthread_barrier_t snapshot_taken;
pthread_barrier_t changed_back;

/* Записывает только поток 1; main читает после pthread_join. */
bool stale_cas_succeeded = false;

/* Обычный CAS-push. Здесь он вызовется только для уже извлечённого A. */
void push(Node *node) {
    Node *old = atomic_load_explicit(&top, memory_order_relaxed);
    do {
        /* Привязываем добавляемый узел к наблюдаемой вершине. */
        node->next = old;
    } while (!atomic_compare_exchange_weak_explicit(
        &top, &old, node, memory_order_release, memory_order_relaxed));
}

/* Учебный CAS-pop. Он не содержит защиты от повторного использования узла.
 * Для простоты в демонстрации его вызывает только поток 2 и только
 * когда поток 1 уже сохранил снимок и НЕ выполняет pop параллельно. */
Node *pop(void) {
    Node *old = atomic_load_explicit(&top, memory_order_acquire);
    while (old != NULL) {
        Node *next = old->next;
        if (atomic_compare_exchange_weak_explicit(
                &top, &old, next, memory_order_acquire, memory_order_acquire)) {
            return old;
        }
        /* При неудаче CAS обновляет old; повторим для новой вершины. */
    }
    return NULL;
}

void *thread_one(void *unused) {
    (void)unused;

    /* Шаг 1. Сделали «снимок» состояния A -> B -> C.
     * old и stale_next — локальные указатели потока 1.
     * Но снимок не защищён от будущих изменений другим потоком! */
    Node *old = atomic_load_explicit(&top, memory_order_acquire); /* A */
    Node *stale_next = old->next; /* B */

    pthread_barrier_wait(&snapshot_taken); /* Разрешаем потоку 2 изменить стек. */
    pthread_barrier_wait(&changed_back);   /* Дожидаемся его последовательности. */

    /* Шаг 3. На вершине снова A, так что CAS проверит A == A и сработает.
     * Но stale_next=B теперь УСТАРЕЛ: реальный next у A уже равен C.
     * Этот CAS не является data race — он атомарный, ошибка ЛОГИЧЕСКАЯ. */
    stale_cas_succeeded = atomic_compare_exchange_strong_explicit(
        &top, &old, stale_next, memory_order_acquire, memory_order_acquire);
    return NULL;
}

void *thread_two(void *unused) {
    (void)unused;
    pthread_barrier_wait(&snapshot_taken);

    /* Шаг 2: специально меняем вершину A -> B -> C на A -> C.
     *   pop A:  head=B, цепочка B -> C;
     *   pop B:  head=C;
     *   push A: A.next=C, head=A.
     * Всё ещё живой узел B теперь НЕ находится в стеке. */
    Node *first = pop();
    Node *second = pop();
    if (first != &a || second != &b) {
        fprintf(stderr, "Unexpected stack state\n");
        abort();
    }
    push(&a);

    pthread_barrier_wait(&changed_back); /* Только теперь разрешаем CAS потока 1. */
    return NULL;
}

int main(void) {
    /* Конструируем список A -> B -> C -> NULL.
     * Присваивания выполняются ДО создания потоков. */
    a = (Node){.id = 'A', .next = &b};
    b = (Node){.id = 'B', .next = &c};
    c = (Node){.id = 'C', .next = NULL};
    atomic_init(&top, &a);

    /* Оба барьера рассчитаны ровно на ДВУХ участников. */
    pthread_barrier_init(&snapshot_taken, NULL, 2);
    pthread_barrier_init(&changed_back, NULL, 2);
    pthread_t t1, t2;
    if (pthread_create(&t1, NULL, thread_one, NULL) != 0 ||
        pthread_create(&t2, NULL, thread_two, NULL) != 0) {
        fprintf(stderr, "pthread_create failed\n");
        return EXIT_FAILURE;
    }
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    /* После неудачной истории CAS фактическая вершина становится B,
     * хотя перед CAS настоящая вершина была A -> C.
     * Вывод «should be A» означает: какой должна была остаться вершина
     * при корректном отклонении попытки по устаревшему снимку. */
    Node *head = atomic_load_explicit(&top, memory_order_acquire);
    printf("ABA: stale CAS succeeded=%d; observed head=%c (should be A)\n",
           (int)stale_cas_succeeded, head ? head->id : '?');

    pthread_barrier_destroy(&snapshot_taken);
    pthread_barrier_destroy(&changed_back);
    /* Здесь «успешен тест» значит ошибка воспроизведена намеренно. */
    return (stale_cas_succeeded && head == &b) ? EXIT_SUCCESS : EXIT_FAILURE;
}
