// Добровольно отдаём физические страницы с помощью MADV_DONTNEED.
#define _DEFAULT_SOURCE

#include "observe.h"
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

enum PROG_STATUS {
    SUCCESS = 0,
    SYSCONF_ERROR = 1,
    MMAP_ERROR = 2,
    MADVISE_ERROR = 3,
    MUNMAP_SUCCESS
};

int main(void) 
{
    long page = sysconf(_SC_PAGESIZE);
    if (page <= 0) { 
        perror("sysconf"); 
        return SYSCONF_ERROR; 
    }

    size_t bytes = 32 * 1024 * 1024;
    void *region = mmap(NULL, bytes, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (region == MAP_FAILED) { 
        perror("mmap"); 
        return MMAP_ERROR; 
    }
    volatile unsigned char *data = region;

    puts("Before touching pages:");
    print_memory();
    Faults before = get_faults();
    for (size_t i = 0; i < bytes; i += (size_t)page) {
        data[i] = 7;
    }
    puts("After writing:");
    print_faults(before);
    print_memory();

    // говорим ОС, что заданные страницы VMA нам не понадобятся
    // в ближайшее время; ОС осовободит соответствующие физические
    // страницы (снижаем RSS)
    if (madvise(region, bytes, MADV_DONTNEED) == -1) {
        perror("madvise"); 
        munmap(region, bytes); 
        return MADVISE_ERROR;
    }
    puts("After MADV_DONTNEED:");
    print_memory();

    before = get_faults();
    unsigned int sum = 0;
    for (size_t i = 0; i < bytes; i += (size_t) page) {
        sum += data[i];
    }
    printf("Read after discard; sum = %u\n", sum); // Анонимные страницы снова нулевые.
    print_faults(before);
    print_memory();

    if (munmap(region, bytes) == -1) { 
        perror("munmap"); 
        return MUNMAP_SUCCESS; 
    }

    return SUCCESS;
}