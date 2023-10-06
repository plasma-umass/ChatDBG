#pragma once

#ifndef JOINSTRINGS_HPP
std::string joinStrings(const std::vector<std::string>& vec) {
    std::ostringstream oss;
    for (size_t i = 0; i < vec.size(); ++i) {
        oss << vec[i];
        if (i != vec.size() - 1) {
            oss << '\n';
        }
    }
    return oss.str();
}
#endif
