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

constexpr static_map<num, keymap, valuemap> map;
constexpr static_map<num, keymap_reverse, valuemap> map_reverse;
constexpr static_table<num, valuemap> table;

int main() {
    std::cout << "First table sorted: " << map.sorted;
    std::cout << "\nSecond table sorted: " << map_reverse.sorted;
    std::cout << "\nThird table sorted: " << table.sorted << '\n';
    unsigned num;
    while (std::cin >> num) {
        try {
            std::cout << "Entry in map 1: " << map[num] << '\n';
        }
        catch (std::exception& e) {
            std::cout << "Not found\n";
        }
        try {
            std::cout << "Entry in map 2: " << map_reverse[num] << '\n';
        }
        catch (std::exception& e) {
            std::cout << "Not found\n";
        }
        //no bounds checking
        std::cout << "Entry in table: " << table[num] << '\n';
    }
    return 0;
}

