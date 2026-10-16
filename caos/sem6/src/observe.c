#include "observe.h"
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>

Faults get_faults(void) {
    struct rusage usage = {0};
    if (getrusage(RUSAGE_SELF, &usage) == -1) {
        perror("getrusage");
        return (Faults){0, 0};
    }
    return (Faults){usage.ru_minflt, usage.ru_majflt};
}

void print_faults(Faults before) {
    Faults after = get_faults();
    printf("  minor faults: +%ld, major faults: +%ld\n",
           after.minor - before.minor, after.major - before.major);
}

void print_memory(void) {
    FILE *file = fopen("/proc/self/status", "r");
    if (file == NULL) {
        perror("/proc/self/status");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), file) != NULL) {
        if (strncmp(line, "VmSize:", 7) == 0 ||
            strncmp(line, "VmRSS:", 6) == 0)
            printf("  %s", line);
    }
    fclose(file);
}
