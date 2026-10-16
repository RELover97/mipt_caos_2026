#ifndef OBSERVE_H
#define OBSERVE_H

typedef struct {
    long minor;
    long major;
} Faults;

Faults get_faults(void);
void print_faults(Faults before);
void print_memory(void);

#endif
