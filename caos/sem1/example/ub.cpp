#include <iostream>
#include <limits>

int main() 
{
    int x = std::numeric_limits<int>::max();

    int y = x + 1;

    std::cout << y << '\n';
}