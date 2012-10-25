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

//forward declaration
template <class T>
class maybe;

//maybe_op* are function-objects that are returned when a
//function is applied to a maybe monad object, which allows
//a more natural syntax.  I've chosen to implement multiple
//classes in order to make it easier to provide type-safety.

//note that these use function pointers, as one of the goals
//of this implementation is to be portable and efficient on
//embedded systems and I'm not sure if just casting everything
//to std::function is the optimal solution on all platforms.
//also, this should allow the compiler to optimize the whole
//type out and replace it with function calls and chained
//null checks.

//the function pointer is nullptr if the maybe is Nothing,
//as f(Nothing) = Nothing

//for a function that takes a T 
template <class T, class U, class ...Args>
class maybe_op {
	friend class maybe<T>;
public:
    typedef maybe<U> result_type;
    typedef maybe<U>(*pfunc_type)(T, Args...);
	maybe<U> operator()(Args&&... args);
private:
	maybe_op(const maybe<T>&, pfunc_type);
	const maybe<T>& t;
	pfunc_type f;
};

//for a function that takes a const T&
template <class T, class U, class ...Args>
class maybe_op_constref {
	friend class maybe<T>;
public:
    typedef maybe<U> result_type;
    typedef maybe<U>(*pfunc_type)(const T&, Args...);
	maybe<U> operator()(Args&&... args);
private:
	maybe_op_constref(const maybe<T>&, pfunc_type);
	const maybe<T>& t;
	pfunc_type f;
};

//for a function that takes a T&
template <class T, class U, class ...Args>
class maybe_op_ref {
	friend class maybe<T>;
public:
    typedef maybe<U> result_type;
    typedef maybe<U>(*pfunc_type)(T&, Args...);
	maybe<U> operator()(Args&&... args);
private:
	maybe_op_ref(maybe<T>&, pfunc_type);
	maybe<T>& t;
	pfunc_type f;
};

//for a member function of T
template <class T, class U, class ...Args>
class maybe_op_mem {
    friend class maybe<T>;
public:
    typedef maybe<U> result_type;
    typedef maybe<U>(T::*pfunc_type)(Args...);
    maybe<U> operator()(Args&&... args);
private:
    maybe_op_mem(maybe<T>&, pfunc_type f);
    maybe<T>& t;
    pfunc_type f;
};

//for a const member function of T
template <class T, class U, class ...Args>
class maybe_op_mem_const {
    friend class maybe<T>;
public:
    typedef maybe<U> result_type;
    typedef maybe<U>(T::*pfunc_type)(Args...) const;
    maybe<U> operator()(Args&&... args);
private:
    maybe_op_mem_const(const maybe<T>&, pfunc_type f);
    const maybe<T>& t;
    pfunc_type f;
};

//maybe<T>: The maybe monad class
template <class T>
class maybe {
public:
	maybe();
	//copy ctor
	template <class U>
	maybe(const maybe<U>&);
	template <class U>
	maybe(maybe<U>&&);
	template <class U>
	maybe(const U&);
	template <class U>
	maybe(U&&);
	//static constructors
	template <class ...Args>
	static maybe<T> Just(Args&&...);
	static maybe<T> Just(const T&);
	static maybe<T> Just(T&&);
	static maybe<T> Nothing();
	//allow if(maybe) checking...
	operator bool();
	//assignment operators
	template <class U>
	maybe<T>& operator=(const U&);
	template <class U>
	maybe<T>& operator=(U&&);
	template <class U>
	maybe<T>& operator=(const maybe<U>&);
	template <class U>
	maybe<T>& operator=(maybe<U>&&);
	
	//apply function to monad: operator[]
	
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
    //only enable if its a class type
    //note: we need the Self = T argument in order for SFINAE to work,
    //otherwise maybe<U>(Self::*)(Args...) is not dependent on the
    //template function and will get looked up anyway without SFINAE,
    //which causes an error on types like maybe<int>
    template <class U, class ...Args, class Self = T> typename
    enable_if<is_class<Self>::value, maybe_op_mem<Self, U, Args...>>
    ::type operator[](maybe<U>(Self::*)(Args...));
    
    template <class U, class ...Args, class Self = T> typename
    enable_if<is_class<Self>::value, maybe_op_mem_const<Self, U, Args...>>
    ::type operator[](maybe<U>(Self::*)(Args...) const) const;
    
    //get underlying object: TODO: Conditionally (at compile time)
    //throw an exception.
	T& get();
    const T& get() const;
    void invalidate();
    ~maybe();
private:
    //allocate space to make value semantics easy without smart pointers
    //we use a memory buffer instead of an object so we can defer
    //construction of the object.
	alignas(T) std::uint8_t memory[sizeof(T)];
	bool valid;
	//do the actual construction
	template <class ...Args>
	void construct(Args&&...);
};

//maybe_op*: implementation of the function object proxy type

//all of the constructors are pretty simple - just initialization
//the operator()s here all check for a null
//		if null, just return Nothing
//		else, return f(Args...)

template <class T, class U, class ...Args>
maybe_op<T, U, Args...>::maybe_op(const maybe<T>& m, pfunc_type func)
	: t(m), f(func)
{
	//
}

template <class T, class U, class ...Args>
maybe<U> maybe_op<T, U, Args...>::operator()(Args&&... args) {
	return f ? f(t.get(), std::forward<Args>(args)...) : maybe<U>();
}

template <class T, class U, class ...Args>
maybe_op_constref<T, U, Args...>::maybe_op_constref(const maybe<T>& m, pfunc_type func)
	: t(m), f(func)
{
	//
}

template <class T, class U, class ...Args>
maybe<U> maybe_op_constref<T, U, Args...>::operator()(Args&&... args) {
	return f ? f(t.get(), std::forward<Args>(args)...) : maybe<U>();
}

template <class T, class U, class ...Args>
maybe_op_ref<T, U, Args...>::maybe_op_ref(maybe<T>& m, pfunc_type func)
	: t(m), f(func)
{
	//
}

template <class T, class U, class ...Args>
maybe<U> maybe_op_ref<T, U, Args...>::operator()(Args&&... args) {
	return f ? f(t.get(), std::forward<Args>(args)...) : maybe<U>();
}

template <class T, class U, class ...Args>
maybe_op_mem<T, U, Args...>::maybe_op_mem(maybe<T>& m, pfunc_type func)
    : t(m), f(func)
{
    //
}

template <class T, class U, class ...Args>
maybe<U> maybe_op_mem<T, U, Args...>::operator()(Args&&... args) {
    return f ? ((t.get()).*(f))(std::forward<Args>(args)...) : maybe<U>();
}

template <class T, class U, class ...Args>
maybe_op_mem_const<T, U, Args...>::maybe_op_mem_const(const maybe<T>& m, pfunc_type func)
    : t(m), f(func)
{
    //
}

template <class T, class U, class ...Args>
maybe<U> maybe_op_mem_const<T, U, Args...>::operator()(Args&&... args) {
    return f ? ((t.get()).*(f))(std::forward<Args>(args)...) : maybe<U>();
}

//now for the actual maybe class:

//just get the object.  TODO: Make this throw an exception
//on invalid (but keep it conditional on a #define)

//ctors

//EVERY ctor should set valid(false).  IF you want to construct, call
//void maybe<T>::construct(Args...), which will set valid for you.
//IF YOU DO NOT, you will destruct an uninitialized object!

//should this produce a valid or invalid object?
//my guess is invalid would be more intuitive, but I don't know...
template <class T>
maybe<T>::maybe() : valid(false) {
	//
}

template <class T>
template <class U>
maybe<T>::maybe(const maybe<U>& m) : valid(false) {
	if (m) {
		construct(m.get());
	}
}

template <class T>
template <class U>
maybe<T>::maybe(maybe<U>&& m) : valid(false) {
	if (m) {
		construct(std::move(m.get()));
	}
}

template <class T>
template <class U>
maybe<T>::maybe(const U& u) : valid(false) {
	construct(u);
}

template <class T>
template <class U>
maybe<T>::maybe(U&& u) : valid(false) {
	construct(u);
}	

//static constructors:

template <class T>
template <class ...Args>
maybe<T> maybe<T>::Just(Args&&... args) {
	maybe<T> m;
	m.construct(std::forward<Args>(args)...);
	return m;
}

template <class T>
maybe<T> maybe<T>::Just(const T& t) {
	return maybe<T>(t);
}

template <class T>
maybe<T> maybe<T>::Just(T&& t) {
	return maybe<T>(t);
}

template <class T>
maybe<T> maybe<T>::Nothing() {
	return maybe<T>();
}

//allow if(maybe) checking...
template <class T>
maybe<T>::operator bool() { 
	return valid; 
}

//assignment operators
template <class T>
template <class U>
maybe<T>& maybe<T>::operator=(const U& u) {
	construct(u);
	return *this;
}

template <class T>
template <class U>
maybe<T>& maybe<T>::operator=(U&& u) {
	construct(u);
	return *this;
}

template <class T>
template <class U>
maybe<T>& maybe<T>::operator=(const maybe<U>& m) {
	if (m) {
		construct(m.get());
	}
	else {
		invalidate();
	}
	return *this;
}

template <class T>
template <class U>
maybe<T>& maybe<T>::operator=(maybe<U>&& m) {
	if (m) {
		construct(std::move(m.get()));
	}
	invalidate();
	return *this;
}

template <class T>
void maybe<T>::invalidate() {
	if (valid) {
		get().~T();
	}
	valid = false;
}

//TODO: Throw an exception	
template <class T>
T& maybe<T>::get() {
	return *(reinterpret_cast<T*>(&(memory[0])));
}

template <class T>
const T& maybe<T>::get() const {
	return *(reinterpret_cast<const T*>(&(memory[0])));
}

template <class T>
template <class ...Args>
void maybe<T>::construct(Args&&... args) {
	if (valid) {
		//don't want to double-construct
		invalidate();
	}
	new(reinterpret_cast<void*>(&(memory[0]))) T(std::forward<Args>(args)...);
	valid = true;
}

template <class T>
maybe<T>::~maybe() {
	invalidate();
}

//these are the apply operators.  Though the signatures are a bit long,
//they don't do much - just construct a maybe_op* with a function
//pointer - if maybe is valid (i.e. Just T), the arg, else null 

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
