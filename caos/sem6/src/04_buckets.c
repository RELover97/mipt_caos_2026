// Следующий шаг: списки свободных блоков (free lists) по размерам.
#define _DEFAULT_SOURCE // чтобы разрешить использование obsolete sbrk

#include <stddef.h>
#include <stdio.h>
#include <sys/mman.h>

#define CAPACITY 4096

enum PROG_STATUS {
    SUCCESS = 0,
    MMAP_ERROR = 1,
    OUT_OF_MEMORY = 2,
    MUNMAP_ERROR = 3
};

typedef struct FreeBlock {
    struct FreeBlock *next;
} FreeBlock;

const size_t sizes[] = {16, 32, 64, 128};
FreeBlock *free_list[4];
unsigned char *memory;
size_t used;

// Достаточно для Linux x86-64; все размеры корзин кратны выравниванию.
_Static_assert(16 % _Alignof(max_align_t) == 0, "Unsupported alignment");


int bucket_for(size_t bytes) {
    for (int i = 0; i < 4; ++i) {
        if (bytes <= sizes[i]) {
            return i;
        }
    }

    return -1;
}


void* allocate(size_t bytes) 
{
    int i = bucket_for(bytes);

    if (i < 0) {
        return NULL;
    }

    if (free_list[i] != NULL) {
        FreeBlock *block = free_list[i];
        free_list[i] = block->next;
        printf("reuse %zu-byte block\n", sizes[i]);
        return block;
    }

    if (used > CAPACITY || sizes[i] > CAPACITY - used) {
        return NULL;
    }

    void *block = memory + used;
    used += sizes[i];
    printf("new %zu-byte block\n", sizes[i]);

    return block;
}

// Упрощение: вызывающий код обязан передать тот же размерный класс.
void deallocate(void *p, size_t bytes) 
{
    if (p == NULL) return;
    int i = bucket_for(bytes);
    if (i < 0) return;
    FreeBlock *block = p;  // Память свободного блока хранит указатель next.
    block->next = free_list[i];
    free_list[i] = block;
}

int main(void) 
{
    // выделяем CAPACITY байтов в VMA, не связанных с файлом,
    // можно читать и писать, copy-on-write
    memory = mmap(NULL, CAPACITY, PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (memory == MAP_FAILED) { 
        perror("mmap"); 
        return MMAP_ERROR; 
    }

    void *x = allocate(24); // корзина 32
    void *y = allocate(80); // корзина 128

    if (!x || !y) { 
        fputs("Out of arena memory\n", stderr); 
        munmap(memory, CAPACITY); 
        return OUT_OF_MEMORY; 
    }

    deallocate(x, 24);
    void *z = allocate(20); // корзина 32: адрес x
    printf("x = %p, z = %p, reused? %s\n", x, z, x == z ? "yes" : "no");
    deallocate(y, 80);
    deallocate(z, 20);

    // Не проверяем чужие указатели, неверный размер и двойной free.
    if (munmap(memory, CAPACITY) == -1) { 
        perror("munmap"); 
        return MUNMAP_ERROR; 
    }

    return SUCCESS;
}
