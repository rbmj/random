#ifndef MAYBE_H_INC
#define MAYBE_H_INC

template <class T>
class maybe;

template <class T, class U, class ...Args>
class maybe_op {
	friend class maybe<T>;
public:
	maybe<U> operator()(Args... args);
private:
	maybe_op(const maybe<T>&, maybe<U>(*)(maybe<T>, Args...));
	const maybe<T>& t;
	maybe<U>(*f)(maybe<T>, Args...);
};

template <class T, class U, class ...Args>
class maybe_op_constref {
	friend class maybe<T>;
public:
	maybe<U> operator()(Args... args);
private:
	maybe_op_constref(const maybe<T>&, maybe<U>(*)(const maybe<T>&, Args...));
	const maybe<T>& t;
	maybe<U>(*f)(const maybe<T>&, Args...);
};

template <class T, class U, class ...Args>
class maybe_op_ref {
	friend class maybe<T>;
public:
	maybe<U> operator()(Args... args);
private:
	maybe_op_ref(maybe<T>&, maybe<U>(*)(maybe<T>&, Args...));
	maybe<T>& t;
	maybe<U>(*f)(maybe<T>&, Args...);
};

template <class T>
class maybe {
public:
	struct null_t {};
	static null_t null;
	maybe() : obj(T()), valid(false) {}
	maybe(null_t n) : obj(T()), valid(false) {}
	template <class U>
	maybe(const maybe<U>& m) {
		if (m) {
			obj = m.obj;
		}
		else {
			valid = false;
		}
	}
	template <class U>
	maybe(maybe<U>&& m) {
		if (m) {
			obj = std::move(m.obj);
		}
		else {
			valid = false;
		}
	}
	template <class ...Args>
	maybe(Args&& ...args) : obj(std::forward<Args>(args)...), valid(true) {}
	operator bool() { return valid; }
	template <class U>
	maybe<T>& operator=(const U& u) {
		obj = u;
		valid = true;
		return *this;
	}
	template <class U>
	maybe<T>& operator=(U&& u) {
		obj = std::move(u);
		valid = true;
		return *this;
	}
	template <class U>
	maybe<T>& operator=(const maybe<U>& m) {
		if (m) {
			obj = m.obj;
		}
		else {
			valid = false;
		}
		return *this;
	}
	template <class U>
	maybe<T>& operator=(maybe<U>&& m) {
		if (m) {
			obj = std::move(m.obj);
		}
		else {
			valid = false;
		}
		return *this;
	}
	maybe<T>& operator=(null_t n) {
		valid = false;
	}
	template <class U, class ...Args>
	maybe_op<T, U, Args...> operator[](maybe<U>(*func)(maybe<T>, Args...)) const;
	template <class U, class ...Args>
	maybe_op_constref<T, U, Args...> operator[](maybe<U>(*func)(const maybe<T>&, Args...)) const;
	template <class U, class ...Args>
	maybe_op_ref<T, U, Args...> operator[](maybe<U>(*func)(maybe<T>&, Args...));
	T& get() {
		return obj;
	}
private:
	T obj;
	bool valid;
};

template <class T, class U, class ...Args>
maybe<U> maybe_op_null(maybe<T> t, Args... args) {
	return maybe<U>(maybe<U>::null);
}

template <class T, class U, class ...Args>
maybe<U> maybe_op_null_constref(const maybe<T>& t, Args... args) {
	return maybe<U>(maybe<U>::null);
}

template <class T, class U, class ...Args>
maybe<U> maybe_op_null_ref(maybe<T>& t, Args... args) {
	return maybe<U>(maybe<U>::null);
}

template <class T, class U, class ...Args>
maybe_op<T, U, Args...>::maybe_op(const maybe<T>& m, maybe<U>(*func)(maybe<T>, Args...))
	: t(m), f(func)
{
	//
}

template <class T, class U, class ...Args>
maybe<U> maybe_op<T, U, Args...>::operator()(Args... args) {
	return f(t, std::forward<Args>(args)...);
}

template <class T, class U, class ...Args>
maybe_op_constref<T, U, Args...>::maybe_op_constref(const maybe<T>& m, maybe<U>(*func)(const maybe<T>&, Args...))
	: t(m), f(func)
{
	//
}

template <class T, class U, class ...Args>
maybe<U> maybe_op_constref<T, U, Args...>::operator()(Args... args) {
	return f(t, std::forward<Args>(args)...);
}

template <class T, class U, class ...Args>
maybe_op_ref<T, U, Args...>::maybe_op_ref(maybe<T>& m, maybe<U>(*func)(maybe<T>&, Args...))
	: t(m), f(func)
{
	//
}

template <class T, class U, class ...Args>
maybe<U> maybe_op_ref<T, U, Args...>::operator()(Args... args) {
	return f(t, std::forward<Args>(args)...);
}

template <class T>
template <class U, class ...Args>
maybe_op<T, U, Args...> maybe<T>::operator[](maybe<U>(*func)(maybe<T>, Args...)) const {
	//ugly ternary - need a static cast to force the compiler to instantiate the function
	return maybe_op<T, U, Args...>(*this,
		valid ? func : static_cast<maybe<U>(*)(maybe<T>, Args...)>(maybe_op_null<T, U, Args...>));
}

template <class T>
template <class U, class ...Args>
maybe_op_constref<T, U, Args...> maybe<T>::operator[](maybe<U>(*func)(const maybe<T>&, Args...)) const {
	//ditto - force instantiation
	return maybe_op_constref<T, U, Args...>(*this,
		valid ? func : static_cast<maybe<U>(*)(const maybe<T>&, Args...)>(maybe_op_null_constref<T, U, Args...>));
}

template <class T>
template <class U, class ...Args>
maybe_op_ref<T, U, Args...> maybe<T>::operator[](maybe<U>(*func)(maybe<T>&, Args...)) {
	//ditto - force instantiation
	return maybe_op_ref<T, U, Args...>(*this,
		valid ? func : static_cast<maybe<U>(*)(maybe<T>&, Args...)>(maybe_op_null_ref<T, U, Args...>));
}

#endif
