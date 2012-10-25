#ifndef METAPROGRAMMING_H_INC
#define METAPROGRAMMING_H_INC

namespace metaprog {
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

}
#endif
