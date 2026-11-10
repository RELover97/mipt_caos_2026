/*
 * ЗАДАЧА 5. Учебный lock-free стек Трейбера на C11 CAS.
 *
 * ./build/05_stack
 *
 * Стек — структура LIFO: последний добавленный элемент извлекается первым.
 * Обычный стек можно защищать mutex, но здесь общая вершина head меняется
 * атомарным compare-and-swap (CAS), без mutex в push/pop.
 *
 * КРИТИЧЕСКИЕ ОГРАНИЧЕНИЯ этого учебного примера:
 *   1) Узлы выделяются заранее и существуют до join ВСЕХ потоков;
 *   2) каждый узел добавляют ровно один раз, затем его НЕ возвращают в стек;
 *   3) производители завершаются ДО запуска потребителей.
 * Это позволяет обойти ABA и освобождение памяти. Без этих условий
 * наивный CAS-стек не является безопасным универсальным контейнером.
 */
#include <inttypes.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Односвязный список: next указывает на следующий элемент или NULL.
 * Поля value и next обычные: до push узел принадлежит одному потоку;
 * после успешного push мы не модифицируем эти поля. */
typedef struct Node {
    int value;
    struct Node *next;
} Node;

/* Вершина стека — один ОБЩИЙ атомарный указатель.
 * Если head == NULL, стек пуст. Иначе head указывает на первый Node. */
typedef struct {
    _Atomic(Node *) head;
} Stack;

void stack_init(Stack *stack) {
    /* Изначально нет ни одного элемента. */
    atomic_init(&stack->head, NULL);
}

/* PUSH: попытаться положить новый узел на вершину.
 * Предусловие: node ещё НЕ находится в стеке и НЕ используется другим
 * потоком; время жизни узла продолжается до завершения всех потоков. */
void stack_push(Stack *stack, Node *node) {
    /* seen — локальный снимок вершины. Другой поток может изменить head
     * уже сразу после load. Поэтому мы НЕ пишем stack->head = node напрямую. */
    /* acquire здесь нужен НЕ только для адреса: если новый узел будет
     * ссылаться на существующий, мы должны получить публикацию его полей.
     * Тогда через наш последующий release эта видимость передастся
     * потоку, который позже извлечёт нашу новую вершину. */
    Node *seen = atomic_load_explicit(&stack->head, memory_order_acquire);

    do {
        /* Предполагаем, что текущая вершина по-прежнему seen.
         * Если CAS победит, цепочка станет node -> seen -> ...
         * Пока node не опубликован через head, его next можно менять. */
        node->next = seen;

        /* Неделимо проверяем head == seen и при равенстве ставим head = node.
         * Успех: release публикует поля node (value, next), а acquire
         * получает публикацию узла, на который указывает node->next.
         * Неудача (failure acquire): CAS переписывает seen фактическим
         * head и получает публикацию этого узла; повторяем,
         * чтобы node->next указывал на НОВУЮ вершину.
         * weak-CAS иногда возвращает false и без конкуренции — цикл нужен. */
    } while (!atomic_compare_exchange_weak_explicit(
        &stack->head, &seen, node,
        memory_order_acq_rel, memory_order_acquire));
}

/* POP: убрать вершину и вернуть указатель на извлечённый узел.
 * Если стек пуст, вернуть NULL.
 * В отличие от «готовых» lock-free контейнеров здесь НЕЛЬЗЯ освобождать
 * или повторно добавлять извлечённые узлы, пока другие потоки работают. */
Node *stack_pop(Stack *stack) {
    /* acquire позволяет видеть инициализированные поля узла, ранее
     * опубликованные другим потоком через release в stack_push. */
    Node *seen = atomic_load_explicit(&stack->head, memory_order_acquire);

    while (seen != NULL) {
        /* Чтобы снять вершину, надо сделать head = seen->next.
         * Разыменование seen безопасно ТОЛЬКО благодаря правилу, что память
         * всех узлов остаётся живой и next не переписывается после push.
         * В универсальном стеке здесь понадобилась бы защита reclamation. */
        Node *next = seen->next;

        /* Если другой поток уже поменял head, эта попытка НЕ тронет стек,
         * CAS сообщит неудачу и запишет текущий head обратно в seen.
         * При успехе именно этот поток «забрал» узел seen; другие pop
         * не смогут успешно снять ту же вершину (при наших ограничениях). */
        if (atomic_compare_exchange_weak_explicit(
                &stack->head, &seen, next,
                memory_order_acquire, memory_order_acquire)) {
            return seen;
        }

        /* Неудача CAS: seen теперь новое значение head (в том числе NULL).
         * Цикл перечитает указатель next для новой вершины. */
    }

    /* NULL означает «на момент наблюдения вершина пуста».
     * В этом проекте к началу pop все производители уже завершились,
     * поэтому новых push позже не будет — потребитель вправе закончить. */
    return NULL;
}

enum { PRODUCERS = 4, CONSUMERS = 4, NODES_PER_PRODUCER = 5000 };
#define TOTAL_NODES (PRODUCERS * NODES_PER_PRODUCER)

/* Общие объекты. Массив содержит ВСЕ узлы с непересекающимися индексами.
 * Каждый producer отвечает за свой участок и не пишет в чужие узлы. */
Stack stack;
Node *nodes;

/* Результаты попов обновляют сразу несколько потоков.
 * Поэтому для счётчика и суммы используем атомарные fetch_add, а не ++/+=.
 * relaxed достаточно: эти числа не служат сигналом о других данных. */
_Atomic uint64_t popped_sum = ATOMIC_VAR_INIT(0);
_Atomic int popped_count = ATOMIC_VAR_INIT(0);

void *push_worker(void *arg) {
    /* pthread_create передаёт адрес ids[i] (см. main).
     * Значение id уникально у каждого producer. */
    int worker_id = *(int *)arg;
    for (int j = 0; j < NODES_PER_PRODUCER; ++j) {
        /* Разбиение массива на непересекающиеся блоки устраняет data race:
         * каждый узел инициализируется только одним производителем. */
        int idx = worker_id * NODES_PER_PRODUCER + j;
        nodes[idx].value = idx + 1;

        /* stack_push сначала установит next, затем опубликует узел через head. */
        stack_push(&stack, &nodes[idx]);
    }
    return NULL;
}

void *pop_worker(void *unused) {
    (void)unused;
    for (;;) {
        Node *node = stack_pop(&stack);
        if (node == NULL) {
            /* Здесь это окончание работы: все producer уже завершились,
             * новые узлы не появятся. При параллельных push такой вывод
             * из NULL был бы НЕВЕРЕН — стек может временно быть пустым. */
            break;
        }

        /* После успешного pop этот узел логически принадлежит нам.
         * Учитываем его value атомарно, потому что другие потребители
         * одновременно увеличивают те же общие счётчики. */
        atomic_fetch_add_explicit(&popped_sum, (uint64_t)node->value,
                                  memory_order_relaxed);
        atomic_fetch_add_explicit(&popped_count, 1, memory_order_relaxed);
    }
    return NULL;
}

int main(void) {
    stack_init(&stack);

    /* Atomic не всегда означает аппаратно lock-free.
     * Проверяем конкретный атомарный УКАЗАТЕЛЬ на нашей платформе.
     * Если реализация под капотом использует lock, учебный пример не
     * может честно называться lock-free на этой машине. */
    if (!atomic_is_lock_free(&stack.head)) {
        fprintf(stderr, "The platform does not provide lock-free atomic pointers\n");
        return EXIT_FAILURE;
    }

    /* Все узлы выделяем ОДНИМ массивом до запуска потоков.
     * calloc обнуляет память; каждый producer позже запишет value/next.
     * Выделение памяти НЕ входит в push/pop. */
    nodes = calloc(TOTAL_NODES, sizeof(*nodes));
    if (nodes == NULL) {
        perror("calloc");
        return EXIT_FAILURE;
    }

    pthread_t writers[PRODUCERS], readers[CONSUMERS];
    int ids[PRODUCERS];

    /* Фаза 1: несколько производителей ОДНОВРЕМЕННО меняют head через CAS.
     * Сначала записываем ids[i], затем передаём АДРЕС этой ячейки потоку.
     * main не меняет ids[i] впоследствии — адрес остаётся действительным. */
    for (int i = 0; i < PRODUCERS; ++i) {
        ids[i] = i;
        if (pthread_create(&writers[i], NULL, push_worker, &ids[i]) != 0) {
            fprintf(stderr, "pthread_create writer failed\n");
            return EXIT_FAILURE;
        }
    }

    /* join гарантирует, что ВСЕ push завершены до запуска потребителей.
     * Это упрощает критерий завершения: когда pop вернул NULL, всё забрано. */
    for (int i = 0; i < PRODUCERS; ++i) {
        pthread_join(writers[i], NULL);
    }

    /* Фаза 2: несколько потребителей ОДНОВРЕМЕННО пытаются снять head.
     * Их pop могут конфликтовать, но один успешный CAS снимает один узел. */
    for (int i = 0; i < CONSUMERS; ++i) {
        if (pthread_create(&readers[i], NULL, pop_worker, NULL) != 0) {
            fprintf(stderr, "pthread_create reader failed\n");
            return EXIT_FAILURE;
        }
    }
    for (int i = 0; i < CONSUMERS; ++i) {
        pthread_join(readers[i], NULL);
    }

    /* К этому моменту все потоки завершились, можно проверить результат.
     * Значения 1..TOTAL_NODES должны встретиться по одному разу;
     * сумма этих чисел = N*(N+1)/2. Сама сумма НЕ является строгим
     * доказательством корректности всех возможных расписаний потоков. */
    int count = atomic_load_explicit(&popped_count, memory_order_relaxed);
    uint64_t sum = atomic_load_explicit(&popped_sum, memory_order_relaxed);
    uint64_t expected_sum = (uint64_t)TOTAL_NODES * (TOTAL_NODES + 1) / 2;
    printf("popped=%d/%d sum=%" PRIu64 "/%" PRIu64 " lock_free=%d\n",
           count, TOTAL_NODES, sum, expected_sum,
           atomic_is_lock_free(&stack.head) ? 1 : 0);

    bool ok = (count == TOTAL_NODES && sum == expected_sum &&
               atomic_load_explicit(&stack.head, memory_order_acquire) == NULL);

    /* Освобождаем память лишь после join, иначе параллельный pop мог бы
     * разыменовать уже освобождённый узел (use-after-free). */
    free(nodes);
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
