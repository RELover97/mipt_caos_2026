// Отображение файла: MAP_SHARED, MAP_PRIVATE, writeback и ftruncate.
#define _DEFAULT_SOURCE // разрешить использование obsolete sbrk

#include "observe.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

enum PROG_STATUS {
    SUCCESS = 0,
    SYSCONF_ERROR = 1,
    MKSTEMP_ERROR = 2,
    FTRUNCATE_ERROR = 3,
    MMAP_ERROR = 4,
};

int main(void) 
{
    long page = sysconf(_SC_PAGESIZE);
    if (page <= 0) { 
        perror("sysconf"); 
        return SYSCONF_ERROR; 
    }

    size_t bytes = 3 * (size_t)page;
    char path[] = "/tmp/memory-lab-XXXXXX";
    int fd = mkstemp(path); // create a tmp file
    if (fd == -1) { 
        perror("mkstemp"); 
        return MKSTEMP_ERROR; 
    }

    unlink(path); // Временный файл исчезнет после close.

    if (ftruncate(fd, (off_t) bytes) == -1) { 
        perror("ftruncate"); 
        close(fd); 
        return FTRUNCATE_ERROR; 
    }

    char *shared = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared == MAP_FAILED) { 
        perror("mmap shared"); 
        close(fd);
        return MMAP_ERROR; 
    }

    Faults before = get_faults();
    shared[0] = 'A';
    shared[page] = 'B';
    puts("After MAP_SHARED writes:");
    print_faults(before);

    char result;
    if (pread(fd, &result, 1, 0) != 1) { 
        perror("pread"); 
        goto fail_shared; 
    }

    printf("pread sees: %c\n", result);
    // Видимость данных не означает, что они уже записаны на носитель.
    if (msync(shared, bytes, MS_SYNC) == -1) { 
        perror("msync"); 
        goto fail_shared; 
    }

    if (fsync(fd) == -1) { 
        perror("fsync"); 
        goto fail_shared; 
    }

    char *private_data = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (private_data == MAP_FAILED) { 
        perror("mmap private"); 
        goto fail_shared; 
    }
    private_data[0] = 'P'; // Copy-on-write: файл не изменяется.

    if (pread(fd, &result, 1, 0) != 1) { 
        perror("pread"); 
        goto fail_private; 
    }
    printf("MAP_PRIVATE = %c, file = %c\n", private_data[0], result);
    
    if (ftruncate(fd, page) == -1) { 
        perror("ftruncate shrink"); 
        goto fail_private; 
    }
    puts("File shrunk to one page. Accessing page 2 now would cause SIGBUS.");

    munmap(private_data, bytes);
    munmap(shared, bytes);
    close(fd);

    return 0;

fail_private:
    munmap(private_data, bytes);
fail_shared:
    munmap(shared, bytes);
    close(fd);

    return 1;
}
