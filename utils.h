#pragma once

#include <string>

void trim(std::string& str) {
    while (!str.empty() && isspace(str.front())) {
        str.erase(0, 1);
    }
    while (!str.empty() && isspace(str.back())) {
        str.pop_back();
    }
}
