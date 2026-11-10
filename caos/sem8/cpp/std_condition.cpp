/*
 * ДОПОЛНЕНИЕ: та же идея condition variable средствами C++17.
 * Основной язык курса — C, это лишь короткое сравнение синтаксиса.
 * Запуск: ./build/cpp_std_condition
 */
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

int main() {
    /* В C++ std::mutex похож на pthread_mutex_t,
     * std::condition_variable — на pthread_cond_t. */
    std::mutex mutex;
    std::condition_variable not_empty;

    /* Состояние защищаем одним и тем же mutex в ОБОИХ потоках.
     * full=false — пока нечего читать; value=0 пока не несёт данных. */
    bool full = false;
    int value = 0;

    /* Независимый атомарный счётчик: нужен для сравнения со stdatomic.h в C. */
    std::atomic<int> updates{0};

    std::thread consumer([&] {
        /* [&] позволяет потоку обращаться к локальным объектам main по ссылке.
         * Они существуют до join(), поэтому время жизни корректно. */

        /* unique_lock — объект, который владеет mutex.
         * condition_variable::wait МОЖЕТ временно отпустить этот mutex,
         * а при пробуждении снова его захватывает. */
        std::unique_lock<std::mutex> lock(mutex);

        /* Версия wait(lock, predicate) логически равна:
         *   while (!full) { not_empty.wait(lock); }
         * Поэтому ложные пробуждения безопасны. */
        not_empty.wait(lock, [&] { return full; });

        /* Здесь full=true и mutex снова захвачен. */
        std::cout << "C++ consumer received " << value << '\n';
        updates.fetch_add(1, std::memory_order_relaxed);
        /* Деструктор lock автоматически вызовет unlock при выходе из лямбды. */
    });

    std::thread producer([&] {
        {
            /* lock_guard захватывает mutex в конструкторе, освобождает
             * в деструкторе. Для обычной короткой критической секции подходит. */
            std::lock_guard<std::mutex> lock(mutex);
            value = 42; /* СНАЧАЛА пишем данные... */
            full = true; /* ...затем публикуем истинный предикат. */
        } /* mutex освобождён до уведомления. Это корректно для этого протокола. */

        /* notify_one НЕ хранится «на потом»: если потребитель ещё не ждёт,
         * он сам увидит full=true под mutex и не вызовет wait. */
        not_empty.notify_one();
    });

    /* std::thread::join выполняет ту же роль, что pthread_join в C:
     * не даёт main завершиться раньше порождённых потоков. */
    producer.join();
    consumer.join();

    std::cout << "C++ atomic updates=" << updates.load() << '\n';
    return updates.load() == 1 ? 0 : 1;
}
