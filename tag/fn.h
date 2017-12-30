#ifndef TAG_FN_H
#define TAG_FN_H

#include <functional>
#include <iterator>

namespace std {
/// a missing type for passing in things by reference, should be in STL
/// needs to be in std namespace, because it is a specialization of
/// binary_function defined in <functional>
template <class Arg1, class Arg2, class Result>
struct binary_function<Arg1, Arg2&, Result> {
    typedef Arg1 first_argument_type;
    typedef Arg2 second_argument_type;
    typedef Result result_type;
};
}

namespace tag {

///@defgroup functional additional functors for <functional>
///This group provides additional functors to complement the <functional> header of STL.
///@ingroup stdpp
//@{

template <typename A, typename m>
struct mem_data_ref_t : std::unary_function<A &, m &> {
    m A::*data;
    inline mem_data_ref_t( m A::*d ) : data(d) {};
    inline m & operator()(A & instance) const {
        return instance.*data;
    }
};

template <typename A, typename m>
struct const_mem_data_ref_t : std::unary_function<const A &, const m &> {
    const m A::*data;
    inline const_mem_data_ref_t( const m A::*d ) : data(d) {};
    inline const m & operator()(const A & instance) const {
        return instance.*data;
    }
};

template <typename A, typename m>
inline struct mem_data_ref_t<A,m> mem_data_ref( m A::*data ){
    return mem_data_ref_t<A,m>(data);
}

template <typename A, typename m>
inline struct const_mem_data_ref_t<A,m> const_mem_data_ref( const m A::*data ){
    return const_mem_data_ref_t<A,m>(data);
}

template <typename A, typename m>
struct mem_data_t : std::unary_function<A *, m &> {
    m A::*data;
    inline mem_data_t( m A::*d ) : data(d) {};
    inline m & operator()(A * instance) const {
        return instance->*data;
    }
};

template <typename A, typename m>
struct const_mem_data_t : std::unary_function<const A *, const m &> {
    const m A::*data;
    inline const_mem_data_t(const m A::*d ) : data(d) {};
    inline const m & operator()(const A * instance) const {
        return instance->*data;
    }
};

template <typename A, typename m>
inline struct mem_data_t<A,m> mem_data( m A::*data ){
    return mem_data_t<A,m>(data);
}

template <typename A, typename m>
inline struct const_mem_data_t<A,m> const_mem_data( const m A::*data ){
    return const_mem_data_t<A,m>(data);
}

template <typename G, typename F>
struct bind_t : std::unary_function<typename F::argument_type, typename G::result_type> {
    const F & f;
    const G & g;
    inline bind_t( const F & f_, const G & g_) :f(f_), g(g_) {};
    inline typename G::result_type operator()( typename F::argument_type a) const {
        return g(f(a));
    }
};

template <typename G, typename F>
inline struct bind_t<G,F> bind( const G & g, const F & f ){
    return bind_t<G,F>(f,g);
}

//@}

///@defgroup iterator additional iterators
///This group provides additional iterators to complement the <iterator> header of STL.
///@ingroup stdpp
//@{

namespace Internal {

template <class T> struct make_const {
    typedef const T type;
};

template <class T> struct make_const<T&> {
    typedef const T & type;
};

template <class A, class B>
struct forward_const {
    typedef B value_type;
    enum { CONST = 0 };
};

template <class A, class B>
struct forward_const<const A, B> {
    typedef typename make_const<B>::type value_type;
    enum { CONST = 1 };
};

template <class A, class B>
struct forward_const<A&, B> {
    typedef B value_type;
    enum { CONST = 2 };
};

template <class A, class B>
struct forward_const<const A&, B> {
    typedef typename make_const<B>::type value_type;
    enum { CONST = 3 };
};

template <class A, class B>
struct forward_const<A *, B> {
    typedef B value_type;
    enum { CONST = 4 };
};

template <class A, class B>
struct forward_const<const A *, B> {
    typedef typename make_const<B>::type value_type;
    enum { CONST = 5 };
};

}

/**
An iterator wrapper that returns a member of a struct the wrapped iterator would point to.
@code
struct simple { int a; float b; };
vector<simple> test;
member_iterator_t<vector<simple>::iterator, int> ita( &simple::a );
ita = test.begin();
cout << *ita; // prints the value of a
@endcode
*/
template <typename It, typename m>
struct member_iterator_t {

    typedef typename std::iterator_traits<It>::value_type ParentValue;

    // iterator defines
    typedef typename std::iterator_traits<It>::iterator_category iterator_category;
    typedef m value_type;
    typedef typename std::iterator_traits<It>::difference_type difference_type;
    typedef typename Internal::forward_const<typename std::iterator_traits<It>::pointer, m *>::value_type pointer;
    typedef typename Internal::forward_const<typename std::iterator_traits<It>::reference, m &>::value_type reference;

    It iterator;
    m ParentValue::*data;

    inline member_iterator_t( m ParentValue::*d ) : data(d) {};
    inline member_iterator_t( const It & it,  m ParentValue::*d ) : data(d) { iterator = it; };

    template <typename Other> inline member_iterator_t & operator=(const Other & other) {
        iterator = other;
        return *this;
    }
    inline member_iterator_t & operator=( const member_iterator_t & other){
        data = other.data;
        iterator = other.iterator;
        return *this;
    }

    inline reference operator*(void) const {
        return (*iterator).*data;
    }
    inline reference operator->(void) const {
        return iterator->*data;
    }

    inline reference operator*(void) {
        return (*iterator).*data;
    }
    inline reference operator->(void) {
        return iterator->*data;
    }


    inline reference operator[](difference_type n) const {
        return iterator[n].*data;
    }
    inline member_iterator_t & operator++() {
        ++iterator;
        return *this;
    }
    inline member_iterator_t operator++(int) {
        member_iterator_t tmp(*this);
        iterator++;
        return tmp;
    }
    inline member_iterator_t & operator--() {
        --iterator;
        return *this;
    }
    inline member_iterator_t operator--(int) {
        member_iterator_t tmp(*this);
        iterator--;
        return tmp;
    }
    inline member_iterator_t & operator+=(difference_type n){
        iterator+=n;
        return *this;
    }
    inline member_iterator_t & operator-=(difference_type n){
        iterator-=n;
        return *this;
    }
    template <typename Other>
    inline difference_type operator-(const Other & other) const {
        return (iterator - other);
    }
    inline difference_type operator-(const member_iterator_t & other) const {
        return (iterator - other.iterator);
    }
    template <typename Other>
    inline bool operator==(const Other & other) const {
        return (iterator == other);
    }
    inline bool operator==(const member_iterator_t & other) const {
        return (iterator == other.iterator);
    }
    template <typename Other>
    inline bool operator!=(const Other & other) const {
        return (iterator != other);
    }
    inline bool operator!=(const member_iterator_t & other) const {
        return (iterator != other.iterator);
    }
};

/**
helper function to simplify the use of @ref member_iterator_t wrapper. This is useful for passing
member iterators as arguments.
@arg it the const iterator to wrap, the new member_iterator_t returned will point to the same position
@arg d the member to wrap
@code
struct simple { int a; float b; };
vector<simple> test;
for_each(member_iterator(test.begin(), &simple::a), member_iterator(test.end(), &simple::a), ... );
@endcode
*/
template <typename It, typename m>
inline struct member_iterator_t<It, m> member_const_iterator( const It & it, m std::iterator_traits<It>::value_type::*d ){
    return member_iterator_t<It, m>(it, d);
}

/*
helper function to simplify the use of @ref member_iterator_t wrapper. This is useful for passing
member iterators as arguments.
@arg it the iterator to wrap, the new member_iterator_t returned will point to the same position
@arg d the member to wrap
@code
struct simple { int a; float b; };
vector<simple> test;
fill(member_iterator(test.begin(), &simple::a), member_iterator(test.end(), &simple::a), 0 );
@endcode
*/

template <typename It, typename m>
inline struct member_iterator_t<It, m> member_iterator(It it, m std::iterator_traits<It>::value_type::*d ){
    return member_iterator_t<It, m>(it, d);
}


//@}

}

#endif // TAG_FN_H
