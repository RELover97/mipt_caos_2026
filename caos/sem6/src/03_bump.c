// Простейший аллокатор: выдаём очередной кусок из общей области mmap.
#define _DEFAULT_SOURCE // чтобы разрешить использование obsolete sbrk

#include <stddef.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

enum PROG_STATUS {
    SUCCESS = 0,
    SBRK_ERROR = 1,
    MMAP_ERROR = 2,
    MUNMAP_ERROR = 3
};

#define CAPACITY 4096

unsigned char *memory;
size_t used;

void *allocate(size_t bytes) {
    size_t align = _Alignof(max_align_t);
    size_t start = (used + align - 1) / align * align;
    if (start > CAPACITY || bytes > CAPACITY - start) {
        return NULL;
    }
    used = start + bytes;
    return memory + start;
}

int main(void) 
{
    void *end = sbrk(0); // только смотрим границу brk-области.
    if (end == (void *) -1) { 
        perror("sbrk"); 
        return SBRK_ERROR; 
    }

    printf("Program break (sbrk(0)): %p\n", end);

    // выделяем в VMA CAPACITY байтов для чтения и записи,
    // устанавливаем copy-on-write свойство, память не связана
    // ни с каким файлом, fd = -1
    memory = mmap(NULL, CAPACITY, PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (memory == MAP_FAILED) { 
        perror("mmap"); 
        return MMAP_ERROR; 
    }

    void *a = allocate(24);
    void *b = allocate(40);

    printf("a = %p, b = %p, used = %zu\n", a, b, used);

    // Освободить один блок нельзя; можно сбросить всю арену.
    used = 0;
    void *c = allocate(24);
    printf("after reset: c = %p, reused a? %s\n", c, c == a ? "yes" : "no");

    if (munmap(memory, CAPACITY) == -1) { 
        perror("munmap"); 
        return MUNMAP_ERROR; 
    }

    printf("PID = %d\n", getpid());
    getchar();

    return SUCCESS;
}
