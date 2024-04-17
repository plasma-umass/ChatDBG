#include <iostream>
#include <string>

int main() {
    // Ask the user for their first name
    std::cout << "Enter your first name: ";
    std::string firstName;
    std::cin >> firstName;
    std::cin.clear();

    // Print the first name back to the user
    std::cout << "Hello, " << firstName << "!" << std::endl;

    // Ask the user for their last name
    std::cout << "Enter your last name: ";
    std::string lastName;
    std::cin >> lastName;
    std::cin.clear();

    // Print the last name back to the user
    std::cout << "Your full name is " << firstName << " " << lastName << std::endl;

    int *p = nullptr;
    *p = 42;

    return 0;
}
