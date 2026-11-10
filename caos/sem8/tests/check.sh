#!/usr/bin/env bash
# Проверки учебных сценариев. Запускать из корня репозитория: make test.
# set -e: завершиться при ошибке любой команды;
# set -u: не допускать использования неинициализированных переменных;
# pipefail: не терять ошибку команды внутри конвейера (если он появится).
set -euo pipefail

# Простейший пример потоков должен завершиться после join.
./build/00_threads

# Mutex-режим обязан дать точный итог 400000.
# unsafe ЗДЕСЬ НЕ ЗАПУСКАЕМ: в нём намеренный data race и UB.
./build/01_counter mutex

# split намеренно воспроизводит потерянное увеличение без data race.
# Успех этой проверки означает, что мы увидели ОЖИДАЕМУЮ учебную ошибку (1),
# а НЕ что алгоритм split пригоден для использования!
./build/02_atomics split
./build/02_atomics cas
./build/02_atomics fetch

# good — корректная односекционная очередь;
# lost — демонстрация неверного протокола ожидания с контролируемым таймаутом.
./build/03_condition good
./build/03_condition lost

# normal — поток может уснуть и быть пробуждён;
# late — значение меняется раньше WAIT, ядро обязано вернуть EAGAIN.
./build/04_futex normal
./build/04_futex late

# Стек считается корректным ТОЛЬКО при описанных в исходнике ограничениях.
./build/05_stack

# ABA — намеренная демонстрация логического повреждения стека.
./build/06_aba

# Необязательное сопоставление стандартных средств C++ с pthread в C.
./build/cpp_01_basics
./build/cpp_02_cas
./build/cpp_std_condition

echo 'OK: all teaching scenarios passed (unsafe data race intentionally excluded)'
