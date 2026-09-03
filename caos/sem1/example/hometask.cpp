#include <iostream>
#include <fstream>

int main() {
    std::ifstream file("numbers.txt");

    int numbers[3];

    for (int i = 0; i <= 3; ++i) {
        file >> numbers[i];
    }

    std::cout << numbers[0] + numbers[1] + numbers[2] << '\n';

    return 0;
}