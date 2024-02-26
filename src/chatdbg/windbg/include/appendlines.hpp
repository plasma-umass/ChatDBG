#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map> // For caching loaded files
#include <algorithm> // For remove
#include <iomanip>   // For setw, left

int append_lines(std::vector<std::string>& lines, const std::string& file_path, int start_line, int end_line) {
    if (start_line < 1 || end_line < start_line) {
        start_line = (std::max)(1, start_line);
        end_line = (std::max)(start_line, end_line);
    }
    
    static std::unordered_map<std::string, std::vector<std::string>> file_cache;
    
    auto iter = file_cache.find(file_path);
    if (iter == file_cache.end()) { // If file not in cache, try to load it
        std::ifstream file(file_path);
        if (!file.is_open()) {
            return 0; // Silently fail if file cannot be opened
        }

        std::vector<std::string> all_lines;
        std::string line;
        while (getline(file, line)) {
            line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
            all_lines.push_back(std::move(line));
        }
        
        iter = file_cache.emplace(file_path, std::move(all_lines)).first;
    }

    const auto& all_lines = iter->second;
    
    if (start_line > static_cast<int>(all_lines.size())) {
        return 0; // Silently fail if start line is greater than the number of lines in the file
    }
    
    // convert start_line to 0-based indexing
    start_line = (std::max)(0, start_line - 1);
    // ensure end_line is within range
    end_line = (std::min)(static_cast<int>(all_lines.size()), end_line);

    int count = 0; // to keep track of the number of lines appended
    for (int i = start_line; i < end_line; ++i) {
        std::stringstream ss;
        ss << "   " << std::setw(6) << std::left << (i + 1) << " " << all_lines[i];
        lines.push_back(ss.str());
        ++count;
    }

    return count;
}

int main() {
    std::string file_path = "path_to_your_file.txt";
    int start_line = 1, end_line = 5;
    std::vector<std::string> lines;

    int lines_appended = append_lines(lines, file_path, start_line, end_line);

    std::cout << "Number of lines appended: " << lines_appended << '\n';
    for (const auto& line: lines) 
        std::cout << line << '\n';

    return 0;
}
