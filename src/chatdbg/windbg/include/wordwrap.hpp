#include <iostream>
#include <sstream>
#include <vector>

std::string word_wrap(const std::string &text, size_t width = 80) {
    std::istringstream stream(text);
    std::string line;
    std::ostringstream wrappedText;
    bool in_code_block = false;
    
    while (std::getline(stream, line)) {
        if (line.starts_with("```")) {
            in_code_block = !in_code_block;
            wrappedText << line << '\n';
            continue;
        }
        
        if (in_code_block) {
            wrappedText << line << '\n';
            continue;
        }

        std::istringstream lineStream(line);
        std::string word;
        size_t lineLength = 0;

        while (lineStream >> word) {
            if (lineLength + word.length() > width && lineLength > 0) {
                wrappedText << '\n';
                lineLength = 0;
            }
            wrappedText << word << ' ';
            lineLength += word.length() + 1; // 1 for the space
        }
        wrappedText << '\n';
    }
    return wrappedText.str();
}
