#ifndef METAPROG_SUPPORT_H_INC
#define METAPROG_SUPPORT_H_INC

#ifndef NO_STDLIB
#include <type_traits>
#include <utility>

namespace metaprog {
	
	template <class T>
	using aligned_memory = std::aligned_storage<sizeof(T), alignof(T)>;
	
	template <class T>
	using forward = std::forward<T>;
	
	template <class T>
	using move = std::move<T>;
	
	template <class T>
	using is_class = std::is_class<T>;
	
	template <bool B, class T = void>
	using enable_if = std::enable_if<B, T>;
	
}

#else

//might still have stdint...
#ifdef HAVE_STDINT_H
#include <stdint.h>

namespace metaprog {
	typedef byte uint_least8_t;

#else 

namespace metaprog {
	typedef byte unsigned char; //fallback should work most of the time
	
#endif

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

    template <bool B, class T = void>
    struct enable_if { };

    template <class T>
    struct enable_if<true, T> {
        typedef T type;
    };

	template <class T>
	typename std::remove_reference<T>::type&& move(T&& t) {
		return static_cast<typename std::remove_reference<T>::type&&>(t);
	}

}

#endif
