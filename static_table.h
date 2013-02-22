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
class static_table;

class static_table_impl {
    template <unsigned N, class KeyGen, class Fn>
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
};

template <unsigned N, class KeyGen, class Fn>
class static_table {
public:
    //public typedefs
    typedef decltype((KeyGen{})(0U)) key_type;
    typedef decltype((Fn{})((KeyGen{})(0U))) value_type;
    typedef static_table<N, KeyGen, Fn> this_type;
    static constexpr unsigned length = N;
private:

    //struct to hold all of the data.  Must be POD and aggregate constructable
    template <bool Sorted>
    struct table_t {
        key_type keys[N];
        value_type values[N];
        
        static constexpr unsigned length = N;
        static constexpr unsigned sorted = Sorted;

        //find the index of key IF sorted (binary search)
        template <bool b = Sorted, class T = this_type>
        constexpr typename std::enable_if<b, unsigned>::type
        index_of(typename T::key_type key, unsigned begin = 0, unsigned end = N)
        {
            // helper macro
            #define ST_MIDPOINT(a, b) (b+((b - a)/2))
            //partition at the ST_MIDPOINT(begin, end)
            return 
            //if this is a real range
            (begin < end) ? 
                //if the partition is the key
                ((keys[ST_MIDPOINT(begin, end)] == key) ? 
                    //return the partition index    
                    ST_MIDPOINT(begin, end) 
                //else
                : 
                    //if the partition is less than the key
                    ((keys[ST_MIDPOINT(begin, end)] < key) ?
                        //then search range after the partition for the key  
                        index_of(key, ST_MIDPOINT(begin, end)+1, end)
                    //else
                    :
                        //search the range before the partition for the key
                        index_of(key, begin, ST_MIDPOINT(begin, end))
                    )
                ) 
            //else, throw an error, as the key is not in the set
            : throw key_not_found_error{};
            //no longer need helper macro
            #undef ST_MIDPOINT
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
    std::enable_if<static_table_impl::comparable<T>::value, bool>::type
    keys_sorted(KeyGen keygen) {
        return (keygen(I2) < keygen(I1)) ?
            false :
            keys_sorted<T, I2, Is...>(keygen);
    }

    //if we can't compare them, then we can't binary search, even if
    //they do happen to be sorted.
    template <class T, unsigned I1, unsigned I2, unsigned... Is>
    static constexpr typename
    std::enable_if<!static_table_impl::comparable<T>::value, bool>::type
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
    static constexpr bool keys_sorted(static_table_impl::seq<Is...>) {
        return keys_sorted<key_type, Is...>(KeyGen());
    }
    

    //now the actual table instance
    const table_t<
        keys_sorted(static_table_impl::gen_seq<N>())
    > table;

    //this does the heavy lifting of generating the table
    template <unsigned... Is>
    constexpr static_table(
        static_table_impl::seq<Is...>,
        KeyGen keygen,
        Fn func
    ) :
        //initialize table - generate all keys, and then all     
        table{ { keygen(Is)... } , { func(keygen(Is))... } }
    {
        //
    }
public:

    //interface to generate a static table
    constexpr static_table() : 
        static_table(static_table_impl::gen_seq<N>(), KeyGen(), Fn()) {}

    constexpr value_type operator[](key_type k) {
        return table[k];
    }

    constexpr value_type at_index(unsigned i) {
        return table.values[i];
    }

    constexpr key_type key_at_index(unsigned i) {
        return table.keys[i];
    }

    static constexpr bool sorted = decltype(table)::sorted;
};

#endif
