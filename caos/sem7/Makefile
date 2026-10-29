# Сборка примеров: каждый файл examples/NN_name/name.c даёт отдельную
# программу build/NN_name/name. Каталог build игнорируется Git.
# -O0 облегчает анализ objdump; -g3 добавляет сведения для отладки.
# -pthread нужен для главы о потоках и допустим для остальных примеров.
# Linux, C11, GCC or Clang. Every example is a separate executable.
CC = cc
CFLAGS = -std=c11 -D_GNU_SOURCE -O0 -g3 -Wall -Wextra -Wpedantic -pthread
LDFLAGS = -pthread

# Находим исходники автоматически; новые главы добавляются без правки списка.
SOURCES := $(wildcard examples/*/*.c)
PROGRAMS := $(patsubst examples/%.c,build/%,$(SOURCES))

.PHONY: all clean list
all: $(PROGRAMS)

# Шаблонное правило: совпавшая часть пути (%) сохраняется в build/.
build/%: examples/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

list:
	@printf '%s\n' $(PROGRAMS)

clean:
	rm -rf build
