#include "static_table.h"
#include <iostream>

constexpr unsigned num = 100;

struct keymap {
    constexpr unsigned operator()(unsigned i) {
        return i;
    }
};

struct keymap_reverse {
    static constexpr unsigned max = num - 1;
    constexpr unsigned operator()(unsigned i) {
        return max - i;
    }
};

struct valuemap {
    constexpr unsigned operator()(unsigned key) {
        return key*key;
    }
};

constexpr static_table<num, keymap, valuemap> table;
constexpr static_table<num, keymap_reverse, valuemap> table_reverse;

int main() {
    std::cout << "First table sorted: " << table.sorted;
    std::cout << "\nSecond table sorted: " << table_reverse.sorted << '\n';
    unsigned num;
    while (std::cin >> num) {
        try {
            std::cout << "Entry in table 1: " << table[num] << '\n';
        }
        catch (std::exception& e) {
            std::cout << "Not found\n";
        }
        try {
            std::cout << "Entry in table 2: " << table[num] << '\n';
        }
        catch (std::exception& e) {
            std::cout << "Not found\n";
        }
    }
    return 0;
}

