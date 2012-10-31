#include <iostream>
#include "maybe.h"

using namespace metaprog;

struct test {
	int value;
	maybe<test> zero() {
		maybe<test> ret = test({value});
		value = 0;
		return ret;
	}
};

maybe<test> test_val(test x) {
	return maybe<test>(x);
}

maybe<test> test_ref(test& x) {
	return x.zero();
}

test test_constref(const test& x) {
	return {x.value + 5};
}

maybe<int> foo(int x) {
	return -x;
}

maybe<int> bar(int& x) {
	return (x = 0);
}

maybe<int> baz(const int& x) {
	return x+5;
}

int main() {
	maybe<int> x = 3;
	maybe<int> one = x[foo]();
	maybe<int> two = x[bar]();
	maybe<int> three = x[baz]();
	maybe<test> y = test({2});
	maybe<test> four = y[test_val]();
	maybe<test> five = y[test_ref]();
	maybe<test> six = y[test_constref]();
	maybe<test> seven = y[&test::zero]();
    maybe<int> nine = x[ [](int a) { return maybe<int>(-a); }]();
    maybe<int> ten = x[[](int& a) -> maybe<int> { return maybe<int>(a = 0); }]();
	maybe<int> eleven = x[[](const int& a) -> maybe<int> { return maybe<int>(a+5); }]();
	std::cout << one.get() << std::endl;
	std::cout << two.get() << std::endl;
	std::cout << three.get() << std::endl;
	std::cout << four.get().value << std::endl;
	std::cout << five.get().value << std::endl;
	std::cout << six.get().value << std::endl;
	std::cout << seven.get().value << std::endl;
	std::cout << nine.get() << std::endl;
	std::cout << ten.get() << std::endl;
	std::cout << eleven.get() << std::endl;
	return 0;
}

