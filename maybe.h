#ifndef MAYBE_H_INC
#define MAYBE_H_INC

#include "metaprog_support.h"

namespace metaprog {

//forward declaration
template <class T>
class maybe;

template <class T>
struct add_maybe {
	typedef maybe<T> type;
};

template <class T>
struct add_maybe<maybe<T>> {
	typedef maybe<T> type;
};

//maybe_op* are function-objects that are returned when a
//function is applied to a maybe monad object, which allows
//a more natural syntax.

//for a generic callable
template <class T, class Callable>
class maybe_op {
	friend class maybe<T>;
private:
	maybe_op(maybe<T>&, Callable);
	maybe_op(maybe<T>&);
	maybe<T>& t;
	Callable call;
public:
    typedef typename add_maybe<decltype(call(t.get()))>::type result_type;
	result_type operator()();
};

//and for const-correctness
template <class T, class Callable>
class maybe_op_const {
	friend class maybe<T>;
private:
	maybe_op_const(const maybe<T>&, Callable);
	maybe_op_const(const maybe<T>&);
	const maybe<T>& t;
	Callable call;
public:
	typedef typename add_maybe<decltype(call(t.get()))>::type result_type;
	result_type operator()();
};

//for a member function of T
template <class T, class U>
class maybe_op_mem {
    friend class maybe<T>;
public:
	typedef typename add_maybe<U>::type result_type;
    typedef U(T::*pfunc_type)();
    result_type operator()();
private:
    maybe_op_mem(maybe<T>&, pfunc_type);
    maybe<T>& t;
    pfunc_type f;
};

//for a const member function of T
template <class T, class U>
class maybe_op_mem_const {
    friend class maybe<T>;
public:
    typedef typename add_maybe<U>::type result_type;
    typedef U(T::*pfunc_type)() const;
    result_type operator()();
private:
    maybe_op_mem_const(const maybe<T>&, pfunc_type);
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
	static maybe<T> just(Args&&...);
	static maybe<T> just(const T&);
	static maybe<T> just(T&&);
	static maybe<T> nothing();
	//allow if(maybe) checking...
	operator bool() const;
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
	template <class Callable>
	maybe_op<T, Callable> operator[](Callable);
	template <class Callable>
	maybe_op_const<T, Callable> operator[](Callable) const;
    //only enable if its a class type
    //note: we need the Self = T argument in order for SFINAE to work,
    //otherwise maybe<U>(Self::*)(Args...) is not dependent on the
    //template function and will get looked up anyway without SFINAE,
    //which causes an error on types like maybe<int>
    template <class U, class Self = T> typename
    enable_if<is_class<Self>::value, maybe_op_mem<Self, U>>
    ::type operator[](U(Self::*)());
    
    template <class U, class Self = T> typename
    enable_if<is_class<Self>::value, maybe_op_mem_const<Self, U>>
    ::type operator[](U(Self::*)() const) const;
    
    //get underlying object: TODO: Conditionally (at compile time)
    //throw an exception.
	T& get();
    const T& get() const;
    void invalidate();
	bool valid() const;
    ~maybe();
private:
    //allocate space to make value semantics easy without smart pointers
    //we use a memory buffer instead of an object so we can defer
    //construction of the object.
	typename aligned_memory<T>::type memory;
	bool is_valid;
	//do the actual construction
	template <class ...Args>
	void construct(Args&&...);
};

//maybe_op*: implementation of the function object proxy type

//all of the constructors are pretty simple - just initialization
//the operator()s here all check for a null
//		if null, just return Nothing
//		else, return f(Args...)

template <class T, class Callable>
maybe_op<T, Callable>::maybe_op(maybe<T>& m, Callable c)
	: t(m), call(c) {
	//
}

template <class T, class Callable>
typename maybe_op<T, Callable>::result_type maybe_op<T, Callable>::operator()() {
	return t.valid() ? call(t.get()) : result_type::nothing();
}

template <class T, class Callable>
maybe_op_const<T, Callable>::maybe_op_const(const maybe<T>& m, Callable c)
	: t(m), call(c)
{
	//
}

template <class T, class Callable>
typename maybe_op_const<T, Callable>::result_type maybe_op_const<T, Callable>::operator()() {
	return t.valid() ? call(t.get()) : result_type::nothing();
}

template <class T, class U>
maybe_op_mem<T, U>::maybe_op_mem(maybe<T>& m, pfunc_type func)
    : t(m), f(func)
{
    //
}

template <class T, class U>
typename maybe_op_mem<T, U>::result_type maybe_op_mem<T, U>::operator()() {
    return t.valid() ? ((t.get()).*(f))() : result_type::nothing();
}

template <class T, class U>
maybe_op_mem_const<T, U>::maybe_op_mem_const(const maybe<T>& m, pfunc_type func)
    : t(m), f(func)
{
    //
}

template <class T, class U>
typename maybe_op_mem_const<T, U>::result_type maybe_op_mem_const<T, U>::operator()() {
    return t.valid() ? ((t.get()).*(f))() : result_type::nothing();
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
maybe<T>::maybe() : is_valid(false) {
	//
}

template <class T>
template <class U>
maybe<T>::maybe(const maybe<U>& m) : is_valid(false) {
	if (m) {
		construct(m.get());
	}
}

template <class T>
template <class U>
maybe<T>::maybe(maybe<U>&& m) : is_valid(false) {
	if (m) {
		construct(move(m.get()));
	}
}

template <class T>
template <class U>
maybe<T>::maybe(const U& u) : is_valid(false) {
	construct(u);
}

template <class T>
template <class U>
maybe<T>::maybe(U&& u) : is_valid(false) {
	construct(u);
}	

//static constructors:

template <class T>
template <class ...Args>
maybe<T> maybe<T>::just(Args&&... args) {
	maybe<T> m;
	m.construct(forward<Args>(args)...);
	return m;
}

template <class T>
maybe<T> maybe<T>::just(const T& t) {
	return maybe<T>(t);
}

template <class T>
maybe<T> maybe<T>::just(T&& t) {
	return maybe<T>(t);
}

template <class T>
maybe<T> maybe<T>::nothing() {
	return maybe<T>();
}

//allow if(maybe) checking...
template <class T>
maybe<T>::operator bool() const { 
	return is_valid; 
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
		construct(move(m.get()));
	}
	invalidate();
	return *this;
}

template <class T>
void maybe<T>::invalidate() {
	if (is_valid) {
		get().~T();
	}
	is_valid = false;
}

template <class T>
bool maybe<T>::valid() const {
	return is_valid;
}

//TODO: Throw an exception	
template <class T>
T& maybe<T>::get() {
	return *(reinterpret_cast<T*>(&memory));
}

template <class T>
const T& maybe<T>::get() const {
	return *(reinterpret_cast<const T*>(&memory));
}

template <class T>
template <class ...Args>
void maybe<T>::construct(Args&&... args) {
	if (is_valid) {
		//don't want to double-construct
		invalidate();
	}
	new(reinterpret_cast<void*>(&memory)) T(forward<Args>(args)...);
	is_valid = true;
}

template <class T>
maybe<T>::~maybe() {
	invalidate();
}

//these are the apply operators.  Though the signatures are a bit long,
//they don't do much - just construct a maybe_op* with a function
//pointer - if maybe is valid (i.e. Just T), the arg, else null 

template <class T>
template <class Callable>
maybe_op<T, Callable> maybe<T>::operator[](Callable c) {
	return maybe_op<T, Callable>(*this, c);
}

template <class T>
template <class Callable>
maybe_op_const<T, Callable> maybe<T>::operator[](Callable c) const {
	return maybe_op_const<T, Callable>(*this, c);
}

template <class T>
template <class U, class Self>
typename enable_if<is_class<Self>::value, maybe_op_mem<Self, U>>
::type maybe<T>::operator[](U(Self::*func)()) {
    return maybe_op_mem<Self, U>(*this, func);
}

template <class T>
template <class U, class Self>
typename enable_if<is_class<Self>::value, maybe_op_mem_const<Self, U>>
::type maybe<T>::operator[](U(Self::*func)() const) const {
    return maybe_op_mem_const<Self, U>(*this, func);
}

}

#endif
