// mmap сначала резервирует виртуальные адреса; страницы выдаются по требованию.
// #define _DEFAULT_SOURCE

#include "observe.h"
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

enum PROG_STATUS {
    SUCCESS = 0,
    SYSCONF_ERROR = 1,
    MMAP_ERROR = 2,
    MUNMAP_ERROR = 3
};

int main(void)
{
    long page = sysconf(_SC_PAGESIZE);
    if (page <= 0) { 
        perror("sysconf"); 
        return SYSCONF_ERROR;
    }

    size_t pages = 4096;
    size_t bytes = pages * (size_t) page;

    puts("Before mmap:");
    print_memory();
    // выделяем bytes байтов в VMA, память не связана ни с каким файлом
    // (fd = -1), чтение и запись разрешаем, copy-on-write
    void *region = mmap(NULL, bytes, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (region == MAP_FAILED) { 
        perror("mmap"); 
        return MMAP_ERROR; 
    }

    puts("After mmap (no writes yet):");
    print_memory();
    printf("Address: %p, page size: %ld, PID: %ld\n", region, page, (long) getpid());

    volatile unsigned char *data = region;
    Faults before = get_faults();
    for (size_t i = 0; i < pages; ++i) {
        data[i * (size_t)page] = 1;
    }
    puts("After first write to each page:");
    print_faults(before);
    print_memory();

    before = get_faults();
    unsigned int sum = 0;
    for (size_t i = 0; i < pages; ++i) {
        sum += data[i * (size_t)page];
    }
    printf("Second pass; sum = %u\n", sum);
    print_faults(before);

    puts("Press Enter to unmap...");
    fflush(stdout);
    getchar(); // Можно посмотреть /proc/<PID>/maps в другом терминале.
    
    if (munmap(region, bytes) == -1) { 
        perror("munmap"); 
        return MUNMAP_ERROR; 
    }

    return SUCCESS;
}
