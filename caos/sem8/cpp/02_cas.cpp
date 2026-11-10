/*
 * Дополнение к src/02_atomics.c: атомарный CAS в стандартном C++17.
 */
#include <atomic>
#include <iostream>
#include <thread>

std::atomic<int> counter{0}; // Общий атомарный объект.

void increase_with_cas() {
    /* Локальный снимок счётчика. Может устареть сразу после чтения. */
    int expected = counter.load(std::memory_order_relaxed);
    for (;;) {
        /* Если counter == expected, неделимо записываем expected+1.
         * Иначе CAS возвращает false и переписывает expected текущим
         * значением counter. На следующей итерации пересчитываем desired.
         * weak может вернуть false и без вмешательства другого потока. */
        if (counter.compare_exchange_weak(expected, expected + 1,
                                          std::memory_order_relaxed,
                                          std::memory_order_relaxed)) {
            break;
        }
    }
}


int main() {
    std::thread first(increase_with_cas);
    std::thread second(increase_with_cas);

    first.join();
    second.join();

    std::cout << "C++ CAS result=" << counter.load() << " expected=2\n";

    return counter.load() == 2 ? 0 : 1;
}
