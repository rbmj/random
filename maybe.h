#ifndef MAYBE_H_INC
#define MAYBE_H_INC

//for SFINAE

template <class T>
struct is_class {
public:
    typedef long class_t;
    typedef char not_class_t;
    //need a double layer of templates in order to
    //use SFINAE to prevent compiler errors
    template <class U>
    static class_t test(int U::*);
    template <class U>
    static not_class_t test(...);
    enum { 
        value = (sizeof(is_class<T>::template test<T>(0)) == sizeof(class_t))
    };
};

template <bool Enable, class T = void>
struct enable_if {};

template <class T>
struct enable_if<true, T> {
    typedef T type;
};

template <class T>
class maybe;

template <class T, class U, class ...Args>
class maybe_op {
	friend class maybe<T>;
public:
    typedef maybe<U> result_type;
    typedef maybe<U>(*pfunc_type)(T, Args...);
	maybe<U> operator()(Args... args);
private:
	maybe_op(const maybe<T>&, pfunc_type);
	const maybe<T>& t;
	pfunc_type f;
};

template <class T, class U, class ...Args>
class maybe_op_constref {
	friend class maybe<T>;
public:
    typedef maybe<U> result_type;
    typedef maybe<U>(*pfunc_type)(const T&, Args...);
	maybe<U> operator()(Args... args);
private:
	maybe_op_constref(const maybe<T>&, pfunc_type);
	const maybe<T>& t;
	pfunc_type f;
};

template <class T, class U, class ...Args>
class maybe_op_ref {
	friend class maybe<T>;
public:
    typedef maybe<U> result_type;
    typedef maybe<U>(*pfunc_type)(T&, Args...);
	maybe<U> operator()(Args... args);
private:
	maybe_op_ref(maybe<T>&, pfunc_type);
	maybe<T>& t;
	pfunc_type f;
};

template <class T, class U, class ...Args>
class maybe_op_mem {
    friend class maybe<T>;
public:
    typedef maybe<U> result_type;
    typedef maybe<U>(T::*pfunc_type)(Args...);
    maybe<U> operator()(Args... args);
private:
    maybe_op_mem(maybe<T>&, pfunc_type f);
    maybe<T>& t;
    pfunc_type f;
};

template <class T, class U, class ...Args>
class maybe_op_mem_const {
    friend class maybe<T>;
public:
    typedef maybe<U> result_type;
    typedef maybe<U>(T::*pfunc_type)(Args...) const;
    maybe<U> operator()(Args... args);
private:
    maybe_op_mem_const(const maybe<T>&, pfunc_type f);
    const maybe<T>& t;
    pfunc_type f;
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
    //cannot use the typedefs for the function pointers below as
    //otherwise the compiler doesn't know what template to
    //instantiate in order to deduce the argument type.  Since
    //we want to use template argument deduction, we have to hard-code
    //the funciton pointer types
	template <class U, class ...Args>
	maybe_op<T, U, Args...> operator[](maybe<U>(*)(T, Args...)) const;
	template <class U, class ...Args>
	maybe_op_constref<T, U, Args...> operator[](maybe<U>(*)(const T&, Args...)) const;
	template <class U, class ...Args>
	maybe_op_ref<T, U, Args...> operator[](maybe<U>(*)(T&, Args...));
    //only eneable if its a class type
    template <class U, class ...Args, class Self = T> typename
    enable_if<is_class<Self>::value, maybe_op_mem<Self, U, Args...>>
    ::type operator[](maybe<U>(Self::*)(Args...));
    template <class U, class ...Args, class Self = T> typename
    enable_if<is_class<Self>::value, maybe_op_mem_const<Self, U, Args...>>
    ::type operator[](maybe<U>(Self::*)(Args...) const) const;
	T& get() {
		return obj;
	}
    const T& get() const {
        return obj;
    }
private:
    //in order to preserve value semantics, use a T and a bool instead of a T*
	T obj;
	bool valid;
};

template <class T, class U, class ...Args>
maybe_op<T, U, Args...>::maybe_op(const maybe<T>& m, pfunc_type func)
	: t(m), f(func)
{
	//
}

template <class T, class U, class ...Args>
maybe<U> maybe_op<T, U, Args...>::operator()(Args... args) {
	return f ? f(t.get(), std::forward<Args>(args)...) : maybe<U>(maybe<U>::null);
}

template <class T, class U, class ...Args>
maybe_op_constref<T, U, Args...>::maybe_op_constref(const maybe<T>& m, pfunc_type func)
	: t(m), f(func)
{
	//
}

template <class T, class U, class ...Args>
maybe<U> maybe_op_constref<T, U, Args...>::operator()(Args... args) {
	return f ? f(t.get(), std::forward<Args>(args)...) : maybe<U>(maybe<U>::null);
}

template <class T, class U, class ...Args>
maybe_op_ref<T, U, Args...>::maybe_op_ref(maybe<T>& m, pfunc_type func)
	: t(m), f(func)
{
	//
}

template <class T, class U, class ...Args>
maybe<U> maybe_op_ref<T, U, Args...>::operator()(Args... args) {
	return f ? f(t.get(), std::forward<Args>(args)...) : maybe<U>(maybe<U>::null);
}

template <class T, class U, class ...Args>
maybe_op_mem<T, U, Args...>::maybe_op_mem(maybe<T>& m, pfunc_type func)
    : t(m), f(func)
{
    //
}

template <class T, class U, class ...Args>
maybe<U> maybe_op_mem<T, U, Args...>::operator()(Args... args) {
    return f ? ((t.get()).*(f))(std::forward<Args>(args)...) : maybe<U>(maybe<U>::null);
}

template <class T, class U, class ...Args>
maybe_op_mem_const<T, U, Args...>::maybe_op_mem_const(const maybe<T>& m, pfunc_type func)
    : t(m), f(func)
{
    //
}

template <class T, class U, class ...Args>
maybe<U> maybe_op_mem_const<T, U, Args...>::operator()(Args... args) {
    return f ? ((t.get()).*(f))(std::forward<Args>(args)...) : maybe<U>(maybe<U>::null);
}

template <class T>
template <class U, class ...Args>
maybe_op<T, U, Args...> maybe<T>::operator[](maybe<U>(*func)(T, Args...)) const {
	return maybe_op<T, U, Args...>(*this, valid ? func : nullptr);
}

template <class T>
template <class U, class ...Args>
maybe_op_constref<T, U, Args...> maybe<T>::operator[](maybe<U>(*func)(const T&, Args...)) const {
	return maybe_op_constref<T, U, Args...>(*this, valid ? func : nullptr);
}

template <class T>
template <class U, class ...Args>
maybe_op_ref<T, U, Args...> maybe<T>::operator[](maybe<U>(*func)(T&, Args...)) {
	return maybe_op_ref<T, U, Args...>(*this, valid ? func : nullptr);
}

template <class T>
template <class U, class ...Args, class Self>
typename enable_if<is_class<Self>::value, maybe_op_mem<Self, U, Args...>>
::type maybe<T>::operator[](maybe<U>(Self::*func)(Args...)) {
    return maybe_op_mem<Self, U, Args...>(*this, valid ? func : nullptr);
}

template <class T>
template <class U, class ...Args, class Self>
typename enable_if<is_class<Self>::value, maybe_op_mem_const<Self, U, Args...>>
::type maybe<T>::operator[](maybe<U>(Self::*func)(Args...) const) const {
    return maybe_op_mem_const<Self, U, Args...>(*this, valid ? func : nullptr);
}

#endif
