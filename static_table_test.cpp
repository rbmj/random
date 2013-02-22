#include "table.h"
#include <iostream>

constexpr double pi = 3.14159;
constexpr double conversion_factor = pi/180;

struct keymap {
    constexpr double operator()(unsigned i) {
        return 90-(i/2.0);
    }
};

struct valuemap {
    constexpr double operator()(double key) {
        return key*conversion_factor;
    }
};

constexpr static_table<180, keymap, valuemap> table;

int main() {
    for (unsigned i = 0; i < 180; ++i) {
        std::cout << table.key_at_index(i) << "\t\t" << table.at_index(i) << '\n';
    }
    std::cout << "Sorted? " << table.sorted << std::endl;
    return 0;
}

