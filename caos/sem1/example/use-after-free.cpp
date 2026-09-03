#include <iostream>

int main() 
{
    int* p = new int(42);

    delete p;

    std::cout << *p << '\n';

    return 0;
}