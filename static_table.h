#ifndef STATIC_TABLE_H_INC
#define STATIC_TABLE_H_INC

#include <type_traits>
#include <stdexcept>

class key_not_found_error : public std::domain_error {
public:
    key_not_found_error() : std::domain_error(
            "Key not found in static lookup table"
    ) {}
};

template <unsigned N, class KeyGen, class Fn>
class static_map;

template <unsigned N, class Fn>
class static_table;

//this class holds components of static_table that are not dependent
//on some or all of static_table's template parameters.
//move into separate class to avoid excess code generation
class static_table_impl {
	//make static_map, static_table friends so it can use our private members
    template <unsigned N, class KeyGen, class Fn>
    friend class static_map;
    template <unsigned N, class Fn>
    friend class static_table;
private:
    //for generating compile time sequences from 0 to N
    template <unsigned... Is> struct seq{};

    template <unsigned X, unsigned... Is>
    struct gen_seq : gen_seq<X-1, X-1, Is...> {};

    template <unsigned... Is>
    struct gen_seq<0, Is...> : seq<Is...> {};

    template <class T>
    class has_lessthan {
        typedef char one;
        typedef struct {char x[2];} two;
        template <class C>
        static one test(decltype(&C::operator<));
        template <class C>
        static two test(...);
    public:
        static constexpr bool value = (sizeof(test<T>(0)) == sizeof(one));
    };

    template <class T>
    struct comparable {
        static constexpr bool value = 
            (has_lessthan<T>::value || std::is_arithmetic<T>::value);
    };
    
    static constexpr unsigned midpoint(unsigned begin, unsigned end) {
		return begin + ((end - begin) / 2);
	}
};

template <unsigned N, class KeyGen, class Fn>
class static_map {
public:
    //public typedefs
    typedef decltype((KeyGen{})(0U)) key_type;
    typedef decltype((Fn{})((KeyGen{})(0U))) value_type;
    typedef static_map<N, KeyGen, Fn> this_type;
    static constexpr unsigned length = N;
private:

	//decrease verbosity
	typedef static_table_impl impl;

    //struct to hold all of the data.  Must be aggregate constructable
    template <bool Sorted>
    struct map_t {
        key_type keys[N];
        value_type values[N];
        
        static constexpr unsigned length = N;
        static constexpr unsigned sorted = Sorted;
		
		//for the next two functions:
		
		//the b template argument is needed to make the enable_if depend
		//on a deduced parameter so SFINAE works
		
		//the T argument is to make it so the function signatures do not
		//conflict and SFINAE will eliminate one version silently
		
        //find the index of key IF sorted (binary search)
        template <bool b = Sorted, class T = this_type>
        constexpr typename std::enable_if<b, unsigned>::type
        index_of(typename T::key_type key, unsigned begin = 0, unsigned end = N)
        {
            //partition at the midpoint
            return 
            //if this is a real range
            (begin < end) ? 
                //if the partition is the key
                ((keys[impl::midpoint(begin, end)] == key) ? 
                    //return the partition index    
                    impl::midpoint(begin, end) 
                //else
                : 
                    //if the partition is less than the key
                    ((keys[impl::midpoint(begin, end)] < key) ?
                        //then search range after the partition for the key  
                        index_of(key, impl::midpoint(begin, end)+1, end)
                    //else
                    :
                        //search the range before the partition for the key
                        index_of(key, begin, impl::midpoint(begin, end))
                    )
                ) 
            //else, throw an error, as the key is not in the set
            : throw key_not_found_error{};
        }
        
        //find the index of key IF unsorted (linear search)
        template <bool b = Sorted, class T = this_type>
        constexpr typename std::enable_if<!b, unsigned>::type 
        index_of(typename T::key_type key, unsigned begin = 0, unsigned end = N)
        {
            return
            //if this is a real range
            (begin < end) ?
                //if the key is at the front of the range
                ((keys[begin] == key) ?
                    //return the front
                    begin
                //else
                : 
                    //look for key in the tail
                    index_of(key, begin+1, end)
                )
            //else, throw an error, as key is not in the set
            : throw key_not_found_error{};
        }
        
        //get the value corresponding to a key
        constexpr value_type operator[](key_type key) {
            //look up the key, get its index, return corresponding value
            return values[index_of(key)];
        }
    };

    //how to determine if the keys are sorted, and thus if the table
    //can use binary search or must do a linear search
    //
    //For SFINAE the U class must be set to T.  We can't default it
    //due to variadic template arguments :(

    //if we can compare them
    template <class T, unsigned I1, unsigned I2, unsigned... Is>
    static constexpr typename
    std::enable_if<impl::comparable<T>::value, bool>::type
    keys_sorted(KeyGen keygen) {
        return (keygen(I2) < keygen(I1)) ?
            false :
            keys_sorted<T, I2, Is...>(keygen);
    }

    //if we can't compare them, then we can't binary search, even if
    //they do happen to be sorted.
    template <class T, unsigned I1, unsigned I2, unsigned... Is>
    static constexpr typename
    std::enable_if<!impl::comparable<T>::value, bool>::type
    keys_sorted(KeyGen) {
        return false;
    }

    //one element is always sorted
    template <class T, unsigned I>
    static constexpr bool keys_sorted(KeyGen) {
        return true;
    }
    
    //proxy overload of keys_sorted
    template <unsigned... Is>
    static constexpr bool keys_sorted(impl::seq<Is...>) {
        return keys_sorted<key_type, Is...>(KeyGen());
    }

    //now the actual table instance
    const map_t<keys_sorted(impl::gen_seq<N>())> map;

    //this does the heavy lifting of generating the table
    template <unsigned... Is>
    constexpr static_map(impl::seq<Is...>, KeyGen keygen, Fn func) :
        //initialize table - generate all keys, and then all values   
        map{ { keygen(Is)... } , { func(keygen(Is))... } }
    {
        //
    }
public:

    //interface to generate a static table
    constexpr static_map() : 
        static_map(impl::gen_seq<N>(), KeyGen(), Fn()) {}

    constexpr value_type operator[](key_type k) {
        return map[k];
    }

    constexpr value_type at_index(unsigned i) {
        return map.values[i];
    }

    constexpr key_type key_at_index(unsigned i) {
        return map.keys[i];
    }

    static constexpr bool sorted = decltype(map)::sorted;
};


template <unsigned N, class Fn>
class static_table {
public:
    //public typedefs
    typedef unsigned key_type;
    typedef decltype((Fn{})(0U)) value_type;
    typedef static_table<N, Fn> this_type;
    static constexpr unsigned length = N;
private:

	//decrease verbosity
	typedef static_table_impl impl;
    //the actual lookup table
    const value_type table[N];

    //this does the heavy lifting of generating the table
    template <unsigned... Is>
    constexpr static_table(impl::seq<Is...>, Fn func) :
        //initialize table - generate values    
        table{ func(Is)... }
    {
        //
    }
public:

    //interface to generate a static table
    constexpr static_table() : 
        static_table(impl::gen_seq<N>(), Fn()) {}

    constexpr value_type operator[](key_type k) {
        return table[k];
    }

    static constexpr bool sorted = true;
};

#endif
