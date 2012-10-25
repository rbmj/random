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
	maybe<test> add(int a) const {
		return maybe<test>(test({a + value}));
	}
};

maybe<test> test_val(test x) {
	return maybe<test>(x);
}

maybe<test> test_ref(test& x) {
	return x.zero();
}

maybe<test> test_constref(const test& x) {
	return x.add(5);
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
	maybe<test> eight = y[&test::add](2);
	std::cout << one.get() << std::endl;
	std::cout << two.get() << std::endl;
	std::cout << three.get() << std::endl;
	std::cout << four.get().value << std::endl;
	std::cout << five.get().value << std::endl;
	std::cout << six.get().value << std::endl;
	std::cout << seven.get().value << std::endl;
	std::cout << eight.get().value << std::endl;
	return 0;
}

