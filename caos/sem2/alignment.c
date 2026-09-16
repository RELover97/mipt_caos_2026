#include <stdio.h>


struct A {
    char f1;
    char p1[3];
    int f2;  // Member with the largest alignment.
    short f3;
    char p3[2];  // sizeof(A) has to be divisible by the largest alignment of any member.
};

struct B {
    char f1;
    char f2;
    long double f3;
};

struct C {
    char f1;
    long double f2;
    char f3;
};

struct D {
    char f1;
    long double f2;
    char f3;
} __attribute__((packed));

int main() 
{
    printf("A: size = %lu; alignment = %lu\n", sizeof(struct A), _Alignof(struct A));
    printf("B: size = %lu; alignment = %lu\n", sizeof(struct B), _Alignof(struct B));
    printf("C: size = %lu; alignment = %lu\n", sizeof(struct C), _Alignof(struct C));
    printf("D: size = %lu; alignment = %lu\n", sizeof(struct D), _Alignof(struct D));


    return 0;
}