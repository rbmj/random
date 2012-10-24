#include <iostream>
#include "maybe.h"

maybe<int> foo(int bar) {
	return -bar;
}

int main() {
	maybe<int> x = 3;
	maybe<int> y = x[foo]();
	std::cout << y.get() << std::endl;
	return 0;
}

