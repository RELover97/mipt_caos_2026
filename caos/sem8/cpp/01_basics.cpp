/*
 * Дополнение к src/00_threads.c и src/01_counter.c, версия C++17.
 */
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

int main() 
{
    int counter = 0;  // Одна общая переменная для всех лямбда-функций.
    std::mutex mutex; // Все потоки используют один объект mutex.
    std::vector<std::thread> threads;

    for (int id = 0; id < 2; ++id) {
        /* [&] даёт доступ к counter и mutex по ссылке (общие объекты).
         * id захватывается по значению: каждый поток получает свой номер. */
        threads.emplace_back([&, id] {
            for (int step = 0; step < 1000; ++step) {
                /* lock_guard берёт mutex сейчас и освободит автоматически
                 * при выходе из этой области видимости, даже при исключении. */
                std::lock_guard<std::mutex> lock(mutex);
                ++counter; // Вся операция защищена блокировкой.
            }
            (void)id; // Можно использовать id в диагностике; пока не нужен.
        });
    }

    /* Пока не выполнен join, счётчик нельзя читать без синхронизации. */
    for (std::thread &thread : threads) {
        thread.join();
    }
    std::cout << "C++ counter=" << counter << " expected=2000\n";
    return counter == 2000 ? 0 : 1;
}
