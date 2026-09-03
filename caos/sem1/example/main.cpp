#include <iostream>
#include "math.h"

#define DEBUG_MESSAGE 1

#if DEBUG_MESSAGE
#define MESSAGE "debug"
#else
#define MESSAGE "release"
#endif

int main()
{
    int x = 7;

    std::cout << "square = " << square(x) << '\n';
    std::cout << "divide = " << divide(20, 4) << '\n';

    std::cout << MESSAGE << '\n';

    return 0;
}