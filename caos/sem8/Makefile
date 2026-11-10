# Makefile учебного проекта на Linux.
# make        — собрать все программы;
# make test   — собрать и выполнить проверки;
# make tsan   — отдельно собрать счётчик с ThreadSanitizer;
# make clean  — удалить результаты компиляции.
# Каждая программа компилируется отдельно, чтобы её было легко изучать.
CC ?= cc
CXX ?= c++
# -std=c11: изучаемые C11 atomics; -pthread: библиотека и режим потоков;
# -g: отладочные символы; -O2: обычные оптимизации;
# -Wall/-Wextra/-Werror: предупреждения помогают найти ошибки.
# _GNU_SOURCE нужен, в частности, для pthread_barrier_t и Linux-примеров.
CFLAGS ?= -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror -pthread -D_GNU_SOURCE
CXXFLAGS ?= -std=c++17 -O2 -g -Wall -Wextra -Wpedantic -Werror -pthread

# wildcard находит все нумерованные C-исходники; patsubst превращает
# src/01_counter.c в исполняемый файл build/01_counter и т. д.
C_SOURCES := $(wildcard src/[0-9][0-9]_*.c)
CPP_SOURCES := $(wildcard cpp/*.cpp)
C_PROGRAMS := $(patsubst src/%.c,build/%,$(C_SOURCES))
CPP_PROGRAMS := $(patsubst cpp/%.cpp,build/cpp_%,$(CPP_SOURCES))

.PHONY: all clean test tsan help
all: $(C_PROGRAMS) $(CPP_PROGRAMS)

build:
	mkdir -p build

# $< — исходный файл (например, src/01_counter.c),
# $@ — имя сборочного результата (например, build/01_counter).
# Условие | build означает: папку build создать до компиляции.
build/%: src/%.c | build
	$(CC) $(CFLAGS) $< -o $@

build/cpp_%: cpp/%.cpp | build
	$(CXX) $(CXXFLAGS) $< -o $@

# Unsafe режим намеренно содержит data race. Запускать отдельно.
tsan: | build
	$(CC) -std=c11 -O1 -g -Wall -Wextra -pthread -D_GNU_SOURCE -fsanitize=thread -fno-omit-frame-pointer src/01_counter.c -o build/01_counter_tsan
	@echo 'Run: ./build/01_counter_tsan unsafe (ожидается диагностика ThreadSanitizer)'

test: all
	bash tests/check.sh

clean:
	rm -rf build

help:
	@echo 'make / make test / make tsan / make clean'
