#include <iostream>
#include <string>
#include <stdexcept>

int main() {
    std::string input;
    std::cout << "Enter an integer: ";
    std::cin >> input;

    int number = std::stoi(input); // Attempt to convert input to an integer
    std::cout << "You entered: " << number << std::endl;

    return 0;
}
