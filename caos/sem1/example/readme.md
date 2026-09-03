Commits:

1. gcc

`g++ -std=c++20 main.cpp math.cpp -o app`

Посмотреть, какие команды gcc запускает под капотом:

`g++ -std=c++20 -v main.cpp math.cpp -o app`

`g++ -std=c++20 -### main.cpp math.cpp -o app`

2. Preprocessor

Добавим директивы в `main.cpp`

`g++ -E main.cpp -o main.i`

Посчитать число строк в файле: 

`wc -l main.cpp`

`wc -l main.i`

3. Compiler

`g++ -std=c++20 -S -masm=intel main.cpp -o main.s`

Ищем `main`

`grep -n "main" main.s`

Скомпилируем программу с разными уровнями оптимизации: 
`O0` = без оптимизаций
`O1` = базовые оптимизации, небольшой рост времени компиляции
`O2` = относительно агрессивная оптимизация, стандарт в продакшн коде
`O3` = агрессивная оптимизация

`g++ -std=c++20 -O0 -S main.cpp -masm=intel -o main.s`

`g++ -std=c++20 -O2 -S main.cpp -masm=intel -o main_O2.s`

Сравним их:

`diff -u main.s main_O2.s`

Исходный код на C++ не определяет конкретную последовательность машинных инструкций. Компилятор оптимизирует программу с учётом выбранных флагов и целевой архитектуры.

Скомпилируем `foo.cpp` с разными уровнями оптимизациями и сравним:

`g++ -O0 -S foo.cpp -masm=intel -o foo_O0.s`

`g++ -O2 -S foo.cpp -masm=intel -o foo_O2.s`