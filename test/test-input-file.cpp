#include <iostream>
#include <fstream>
#include <cstdlib> // For atoi

using namespace std;

int main(int argc, char *argv[]) {
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <integer> <string> <filename>" << endl;
        return 1;
    }

    int integerValue = atoi(argv[1]);
    string stringValue = argv[2];
    string filename = argv[3];

    cout << "Integer value: " << integerValue << endl;
    cout << "String value: " << stringValue << endl;
    cout << "Filename: " << filename << endl;

    // Open and parse the file
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Unable to open file " << filename << endl;
        return 1;
    }

    // Intentional null pointer reference
    int *nullPtr = nullptr;
    *nullPtr = 42; // This will cause a segmentation fault

    file.close();

    return 0;
}
