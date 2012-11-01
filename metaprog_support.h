#ifndef METAPROG_SUPPORT_H_INC
#define METAPROG_SUPPORT_H_INC

namespace metaprog {

template <class Callable, class Arg>
struct unary_result {
private:
	static Arg(*arg)();
	static Callable(*callable)();
public:
	typedef decltype((callable())(arg())) type;
};

}
	

//standard library stuff and some reimplementation below:

#ifndef NO_STDLIB
#include <type_traits>
#include <utility>

namespace metaprog {
	
	template <class T>
	using aligned_memory = std::aligned_storage<sizeof(T), alignof(T)>;
	
	using std::forward;
	
	using std::move;

	using std::is_class;
	
	using std::enable_if;
	
	using std::is_convertible;
	
}

#else

namespace metaprog {
	typedef byte unsigned char; //fallback should work most of the time
	
	//we can't use the standard library...
	
	template <class T>
	struct remove_reference {
		typedef T type;
	};
	
	template <class T>
	struct remove_reference<T&> {
		typedef T type;
	};
	
	template <class T>
	struct remove_reference<T&&> {
		typedef T type;
	};
	
	template <class T>
	T&& forward(typename remove_reference<T>::type& t) {
		return static_cast<T&&>(t);
	}
    
    constexpr unsigned div_roundup(unsigned a, unsigned b) {
        return (a/b) + (((a % b) != 0) ? 1 : 0);
    }

    template <class T>
    struct aligned_memory {
        typedef alignas(T) byte[div_roundup(sizeof(T), sizeof(byte))] type;
    };
    
    template <class T>
	struct is_class {
	private:
		typedef long class_t;
		typedef char not_class_t;
		//need a double layer of templates in order to
		//use SFINAE to prevent compiler errors
		template <class U>
		static class_t test(int U::*);
		template <class U>
		static not_class_t test(...);
	public:
		enum { 
			value = (sizeof(test<T>(0)) == sizeof(class_t))
		};
	};

    template <bool B, class T = void>
    struct enable_if { };

    template <class T>
    struct enable_if<true, T> {
        typedef T type;
    };

	template <class T>
	typename remove_reference<T>::type&& move(T&& t) {
		return static_cast<typename remove_reference<T>::type&&>(t);
	}
	
	template <class From, class To>
	struct is_convertible {
	private:
		typedef char convertible_t;
		typedef long inconvertible_t;
		static From(*func)();
		static convertible_t test(To);
		static inconvertible_t test(...);
	public:
		enum {
			value = (sizeof(test(func())) == sizeof(convertible_t))
		};
	};

}

#endif

#endif

