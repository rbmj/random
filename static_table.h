#ifndef STATIC_TABLE_H_INC
#define STATIC_TABLE_H_INC

#include <type_traits>
#include <stdexcept>

/**
 * Exception class that indicates a given value is not in the map
 */
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

/**
 * Implementation container for static_map, static_table
 * 
 * This class holds components of static_table that are not dependent
 * On some or all of static_table's template parameters.
 *
 * They are moved into a separate class to avoid excess code generation.
 * 
 * Everything in this class is private and either a type or static.
 * static_map and static_table are friends of this class so that they
 * can access these internals (a sort of private namespace).
 */
class static_table_impl {
	//make static_map, static_table friends so it can use our private members
    template <unsigned N, class KeyGen, class Fn>
    friend class static_map;
    template <unsigned N, class Fn>
    friend class static_table;
private:
    /**
     * A compile-time sequence of unsigned integers.
     * 
     * \tparam Is Represents the sequence of unsigned integers
     */
    template <unsigned... Is> struct seq {};
	
	#ifdef DOXYGEN
	/**
	 * A recursive type that generates a sequence from 0 to N
	 * 
	 * gen_seq<N> will be a subclass of seq<0, ... N>
	 * 
	 * \tparam N The upper bound on the sequence
	 */
	template <unsigned N>
	struct gen_seq<N> {};
	#else
	//real code for this:
    template <unsigned N, unsigned... Is>
    struct gen_seq : gen_seq<N-1, N-1, Is...> {};

    template <unsigned... Is>
    struct gen_seq<0, Is...> : seq<Is...> {};
    #endif

	/**
	 * A type-traits class that detects if T can be compared with <
	 * 
	 * \tparam T The type to check
	 */
    template <class T>
	class comparable {
	private:
		///Indicates the first case
		typedef char a;
		///Indicates the second case
		typedef struct {char x[2];} b;
		/**
		 * First case.
		 * 
		 * If the result of comparing two values of type T using < is
		 * implicitly convertible to bool, then this case will be a
		 * better match for test(0) than test(...).
		 */
		template <class C = T>
		static a test(
			typename std::enable_if<
				std::is_convertible<
					decltype(std::declval<C>() < std::declval<C>()),
					bool
				>::value,
				int
			>::type
		);
		/**
		 * Second case.
		 * 
		 * This case will always match test(0), but will be a worse
		 * match than the first case, assuming that the first case is
		 * not eliminated due to SFINAE.
		 */
		template <class C = T>
		static b test(...);
	public:
		/**
		 * Whether T can be compared using <
		 * 
		 * This will be true if the result of a comparison of two values
		 * of type T is implicitly convertible to bool.
		 */
		static constexpr bool value = (sizeof(test(0)) == sizeof(a));
	};
    
    /** 
     * A constexpr function that gets the midpoint of a range
     * 
     * \param begin The start of the range
     * \param end The end of the range
     * \return The midpoint of the range [begin, end)
     */
    static constexpr unsigned midpoint(unsigned begin, unsigned end) {
		return begin + ((end - begin) / 2);
	}
};

/**
 * A statically initialized lookup map.
 * 
 * The KeyGen type must be able to map an unsigned index in the range
 * [0, N) to a key.  The Fn type then maps the output of the KeyGen type
 * to values.  Values can be looked up by key or by index.
 * 
 * Calling an object of type KeyGen or Fn MUST be a constexpr operation.
 * To use the default constructor, KeyGen and Fn MUST be constexpr
 * default constructable function objects.  
 * 
 * To use regular functions, see the global function generate_static_map().
 * 
 * To illustrate:
 * 
 * \code{.cpp}
 * 
 * //An example of types that would be valid:
 * template <typename key_type, typename value_type>
 * struct static_map_functs {
 *     struct KeyGen {
 *         constexpr key_type operator()(unsigned);
 *     };
 *     struct Fn {
 *         constexpr value_type operator()(key_type);
 *     };
 * };
 * 
 * \endcode
 * 
 * If an object of type KeyGen k maps [0, N) in a manner such that
 * k(x) < k(x+1) for all x in [0, N-1), then this class can perform
 * some optimizations in the lookup phase.  If key_type cannot be
 * compared using <, or if the result of such a comparison is not
 * convertible to bool, then this check will be skipped.
 * 
 * \tparam N The number of entries in the lookup table
 * \tparam KeyGen A type that maps a sequence to a series of keys
 * \tparam Fn A type that maps a key to a value
 */
template <unsigned N, class KeyGen, class Fn>
class static_map {
public:
    //public typedefs
    ///The type of the keys in the map
    typedef decltype((KeyGen{})(0U)) key_type;
    ///The type of the mapped values in the map
    //can't assume that key_type is default constructible
    typedef decltype((Fn{})((KeyGen{})(0U))) value_type;
    ///Shorthand form for the type of this object
    typedef static_map<N, KeyGen, Fn> this_type;
    ///The number of (key, value) pairs in the map
    static constexpr unsigned length = N;
private:

	//decrease verbosity
	typedef static_table_impl impl;

    /**
     * A struct to hold all of the data.
     * 
     * This needs to be aggregate constructable so that the outer class'
     * constructor will work and so that we can do everything with the
     * variadic template.  It also defines its own member functions for
     * looking up values to simplify static_map's  implementation.
     * 
     * Though it might be theoretically possible to implement this as
     * a hash table, it would add a significant degree of complexity
     * for the gain of O(1) performance.  Additionally, if performance
     * is critical then users should make pains to ensure that the
     * keys are sorted.  In such a circumstance, N must be very large
     * in order for the overhead of hashing to be faster than a normal
     * binary search.
     * 
     * Long story short - I judged that hashing wasn't worth it.  With
     * constexpr and TMP it's already complex enough!
     * 
     * \tparam Sorted If key[i] < key[i+1] for all i | 0 < i < N
     */
    template <bool Sorted>
    struct map_t {
		///the array of keys
        key_type keys[N];
        ///the array of values such that keys[i] maps to values[i]
        value_type values[N];
        
        ///also can have length here for convenience's sake
        static constexpr unsigned length = N;
        ///so it's easier to refer to sorted outside the template
        static constexpr unsigned sorted = Sorted;
		
		#ifdef DOXYGEN
		/**
		 * Get the index of key in keys.
		 * 
		 * If sorted is true, this performs a binary search of keys for
		 * key.  This should be O(lg(N)) in the worst case.
		 * 
		 * If sorted is false, this performs a linear search of keys
		 * for key.  This should be O(N) in the worst case.
		 * 
		 * \param key The key to search for in keys
		 * \return The index of key in keys (i.e., keys[i] == key)
		 * 
		 * \throws key_not_found_error key is not in this map.
		 */
		constexpr unsigned index_of(key_type key);
		#else
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
        #endif
        
        /**
         * Look up key
         * 
         * Find the value that key maps to in the tables.
         * 
         * \param key The key to map
         * \return The mapped value for key
         * \throws key_not_found_error key is not in this map
         */
        constexpr value_type operator[](key_type key) {
            //look up the key, get its index, return corresponding value
            return values[index_of(key)];
        }
    };

	//don't document this section
	#ifndef DOXYGEN
	
    //how to determine if the keys are sorted, and thus if the table
    //can use binary search or must do a linear search
    //
    //For SFINAE the T must be key_type.  We can't default it
    //due to variadic template arguments :(
    
    //base case:
    template <class T, unsigned I1, unsigned I2, unsigned... Is>
    static constexpr typename
    std::enable_if<impl::comparable<T>::value, bool>::type
    keys_sorted(KeyGen keygen) {
		//check if the front two are out of order.  If they are, we
		//aren't sorted.  If they aren't, check the next two.
        return (keygen(I2) < keygen(I1)) ?
            false :
            keys_sorted<T, I2, Is...>(keygen);
    }

    //special case: we have at least two, and we can't compare them
    template <class T, unsigned I1, unsigned I2, unsigned... Is>
    static constexpr typename
    std::enable_if<!impl::comparable<T>::value, bool>::type
    keys_sorted(KeyGen) {
        return false;
    }

    //special case: one element is always sorted
    template <class T, unsigned I>
    static constexpr bool keys_sorted(KeyGen) {
        return true;
    }
    
    //transforms the sequence into variadic argument list to pass to
    //the other templates
    template <unsigned... Is>
    static constexpr bool keys_sorted(impl::seq<Is...>) {
        return keys_sorted<key_type, Is...>(KeyGen());
    }
    
    #endif
    
    /**
     * Check if the keys are sorted.
     * 
     * \return True if the keys are sorted, false otherwise
     */
    //a pretty proxy that sets up the arguments correctly.
    static constexpr bool keys_sorted() {
		return keys_sorted(impl::gen_seq<N>());
	}

    ///the instance of the map itself.
    const map_t<keys_sorted()> map;

	#ifndef DOXYGEN
    //this does the heavy lifting of generating the table
    template <unsigned... Is>
    constexpr static_map(impl::seq<Is...>, KeyGen keygen, Fn func) :
        //initialize table - generate all keys, and then all values   
        map{ { keygen(Is)... } , { func(keygen(Is))... } }
    {
        //
    }
    #endif
public:

    /**
     * Construct a static_map
     */
    constexpr static_map() : 
        static_map(impl::gen_seq<N>(), KeyGen(), Fn())
    {
		//
	}

	/**
	 * Look up a key in the map.
	 * 
	 * This function is constexpr, so it can be computed at compile
	 * time if k is itself constexpr.
	 * 
	 * The time complexity of this operation is O(lg(N)) if the keys
	 * are sorted (strictly increasing), and O(N) in all other cases.
	 * 
	 * \param k A key
	 * \return The value that k maps to.
	 * 
	 * \throws key_not_found_error key is not in this map
	 */
    constexpr value_type operator[](key_type k) {
        return map[k];
    }

	/**
	 * Get the value at a certain index in the underlying table.
	 * 
	 * This is a lower-level function.  It should not be used often.
	 * It might be useful in certain situation for optimization
	 * purposes or in unusual use cases.
	 * 
	 * Conceptually, this is equivalent to doing a lookup on KeyGen and
	 * then a lookup on Fn.  Its time complexity is O(1).
	 * 
	 * This function, being lower level, is not bounds checked.
	 * 
	 * \param i The index
	 * \return The ith value stored/mapped to in this map.
	 */
    constexpr value_type at_index(unsigned i) {
        return map.values[i];
    }
	
	/**
	 * Get the key at a certain index in the underlying table.
	 * 
	 * This is a lower-level function.  It should not be used often.
	 * It might be useful in certain situation for optimization
	 * purposes or in unusual use cases.
	 * 
	 * This function in particular might be useful for reverse
	 * lookups.  Additionally, this can be viewed as a lookup table for
	 * KeyGen.  Its time complexity is O(1).
	 * 
	 * This function, being lower level, is not bounds checked.
	 * 
	 * \param i The index
	 * \return The ith key stored/mapped in this map.
	 */
    constexpr key_type key_at_index(unsigned i) {
        return map.keys[i];
    }
	
	///if the keys are sorted
    static constexpr bool sorted = decltype(map)::sorted;
};

/**
 * A statically initialized lookup table.
 * 
 * The Fn type is a function type that maps an unsigned index to a
 * value.
 * 
 * This class can be thought of as an optimized version of static_map
 * where KeyGen is the identity function.
 * 
 * Calling an object of type Fn MUST be a constexpr operation, and Fn
 * MUST be a constexpr default constructable function object.
 * 
 * To use regular functions, see the global function generate_static_table().
 * 
 * To illustrate:
 * 
 * \code{.cpp}
 * 
 * //this may be used with either constructor
 * template <typename value_type>
 * struct Fn {
 *     constexpr value_type operator()(unsigned);
 * };
 * 
 * \endcode
 * 
 * \tparam N The number of entries in the lookup table
 * \tparam Fn A type that maps a key to a value
 * 
 */
template <unsigned N, class Fn>
class static_table {
public:
    //public typedefs
	///The type of the keys in the table (for compatibility with static_map)
    typedef unsigned key_type;
    ///The type of the values in the table
    typedef decltype((Fn{})(0U)) value_type;
	///Shorthand form for the type of this object
    typedef static_table<N, Fn> this_type;
    ///The number of values in the table
    static constexpr unsigned length = N;
private:

	//decrease verbosity
	typedef static_table_impl impl;
	
    ///the actual lookup table
    const value_type table[N];

	#ifndef DOXYGEN
    //this does the heavy lifting of generating the table
    template <unsigned... Is>
    constexpr static_table(impl::seq<Is...>, Fn func) :
        //initialize table - generate values    
        table{ func(Is)... }
    {
        //
    }
    
    //make this private to maintain consistency & compatibility with
    //static_map.  It's more intuitive if there's just *one* set of
    //rules to follow.
    
    explicit constexpr static_table(Fn func) :
		static_table(impl::gen_seq<N>(), func)
	{
		//
	}
    #endif
public:

    /**
     * Construct a static_table
     */
    constexpr static_table() : 
        static_table(impl::gen_seq<N>(), Fn()) 
    {
		//
	}

	/**
	 * Look up an index in the table.
	 * 
	 * This function is constexpr, so it can be computed at compile
	 * time if i is itself constexpr.
	 * 
	 * The time complexity of this operation is O(1).
	 * 
	 * This function is not bounds checked.
	 * 
	 * \param i An index
	 * \return The value at i.
	 */
    constexpr value_type operator[](unsigned i) {
        return table[i];
    }
    	
    /**
	 * Look up an index in the table.
	 * 
	 * This function is exactly the same as operator[]().  It is provided
	 * for convenience and compatibility with static_map.
	 * 
	 * This function is constexpr, so it can be computed at compile
	 * time if i is itself constexpr.
	 * 
	 * The time complexity of this operation is O(1).
	 * 
	 * This function is not bounds checked.
	 * 
	 * \param i An index
	 * \return The value at i.
	 */
    constexpr value_type at_index(unsigned i) {
		return table[i];
	}
	
	/**
	 * Find the index of a key
	 * 
	 * This function is simply the identity function.  It is provided
	 * for interface compatibility with static_map.
	 * 
	 * \param i The index
	 * \return i
	 */
	constexpr unsigned key_at_index(unsigned i) {
		return i;
	}

	///if the keys are sorted - provided for compatibility with static_map
    static constexpr bool sorted = true; //always true (0...N always sorted)
};

#endif
