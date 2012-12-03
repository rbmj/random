#include "pipe.h"
#include <iostream>

int f(int x) {
    return x*x;
}

int g(int x) {
    return x-2;
}

int h(int x) {
    return x/2;
}

int main() {
    auto foo = metaprog::pipe_source<int>::create(f, g);
    auto bar = metaprog::pipe_source<int>()[g][h];
    std::cout << foo(10) << std::endl << bar(10) << std::endl;
    return 0;
}

