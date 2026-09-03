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

4. Object file

`g++ -std=c++20 -c main.cpp -o main.o`

`g++ -std=c++20 -c math.cpp -o math.o`

Информация о файле

- тип содержимого файла (обычно не по расширению, а по магическим байтам)
- например, ELF-файл начинается с байтов `7f 45 4c 46`, где `45 4c 46` — ASCII "ELF".
- LSB (least significant byte first) = little endian
- relocatable = object file, not app

`file main.o`

`file math.o`

Объектный файл содержит:
- машинный код;
- таблицу символов;
- информацию о секциях;
- relocation entries (места, которые линкеру нужно исправить после компоновки объектных файлов)
- ссылки на функции/переменные, адреса которых ещё не определены окончательно.

Посмотреть символы файла

`nm main.o`

`T` — функция/символ, определённый здесь;
`U` — undefined symbol, который должен быть разрешён linker'ом.
`r` - relocation entry

`readelf -s main.o`

Посмотреть relocation entries:

`readelf -r main.o`

Здесь:
`Offset` — где в секции находится поле, которое надо исправить;
`Type` — как именно вычислять новое значение;
`Sym. Name` — относительно какого символа производится relocation;
`Addend` — дополнительная константа в формуле.

Посмотреть секции файла:

`readelf -S main.o`

Секции файла (= способ организации содержимого файла) могут быть:

- `.text` -  машинный код

- `.data` - инициализированные глобальные / статические данные

- `.bss` - неинициаилизированные глобальные / статические данные

- `.rodata` - константные данные

- `.symtab` - таблица символов

- `.strtab` - строки, используемыее таблицей символов

- `.rela.text`, `.rela.data` - relocation entries

5. Linkage

`g++ main.o -o app` -> error

`g++ main.o math.o -o app`

`./app`

`nm app`

6. gdb

`g++ -std=c++20 -g -O0 main.cpp math.cpp -o app_debug`

`-g` = add debug information

`gdb ./app_debug`

In GDB:

`break main`
`run`
`list`
`next`
`print x`

`next` v.s. `step`:

`next` = next line
`step` = step inside

`backtrace` / `bt` - call stack

`info locals` -  local vars (have a look at program state in running point)

Change square

`g++ -std=c++20 -g -O0 main.cpp math.cpp -o app_debug`

`gdb ./app_debug`

In GDB:

`break square`
`run`
`next`
`watch result` (only after `result` is allocated on stack!)
`continue`

Теперь GDB остановится, когда `result` изменится.

`watchpoint` отвечает на вопрос «когда изменилась эта переменная?»

7. Find error via gdb

Change `math.cpp` and `main.cpp`

`g++ -std=c++20 -g -O0 main.cpp math.cpp -o app_debug`

`./app_debug` -> segmentation fault

`gdb ./app_debug`

`run`

`bt`

8. Sanitizers

GDB отвечает на вопрос "Что происходило с программой в момент выполнения?"

Sanitizer отвечает на вопрос "Есть ли в программе определённые классы ошибок выполнения?"

Sanitizers:

- AddressSanitizer:

`-fsanitize=address`

для обнаружения out-of-bounds и use-after-free

- UndefinedBehaviorSanitizer:

`-fsanitize=undefined`

для обнаружения UB

Out of bounds task

`g++ -std=c++20 -g -O0 out-of-bounds.cpp -o out-of-bounds`

`./out-of-bounds`

Программа может напечатать мусор, упасть или даже «нормально» завершиться.

`g++ -std=c++20 -g -O1 -fsanitize=address -fno-omit-frame-pointer out-of-bounds.cpp -o out-of-bounds_asan`

`-fno-omit-frame-pointer` — это флаг gcc, который запрещает компилятору удалять frame pointer (RBP) из функций (нужно для упрощения работы sanitizers)

`./out-of-bounds_asan`

Use after free

`g++ -g -O1 -fsanitize=address -fno-omit-frame-pointer use-after-free.cpp -o use-after-free_asan`

`./use-after-free_asan`