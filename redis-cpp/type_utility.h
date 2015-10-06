#ifndef TYPE_UTILITY_H
#define TYPE_UTILITY_H

#include <utility>
#include <type_traits>
#include <iterator>

namespace detail {

// copy and paste from lld core library
using std::begin;
using std::end;

struct undefined_begin {};
struct undefined_end {};

template <typename R> class begin_result {
    template <typename T> static auto check(T &&t) -> decltype(begin(t));
    static undefined_begin check(...);
public:
    typedef decltype(check(std::declval<R>())) type;
};

template <typename R> class end_result {
    template <typename T> static auto check(T &&t) -> decltype(end(t));
    static undefined_end check(...);
public:
    typedef decltype(check(std::declval<R>())) type;
};

} // namespace detail

// This is not a really range check, it just checks whether the type has
// corresponding begin and end free function and their type is same.
// Because C++17 will introduce a real range concept supported by concept lite, 
// those kinds of hack could be removed later.
template <typename R>
struct is_range : std::is_same<typename detail::begin_result<R>::type,
    typename detail::end_result<R>::type> {};


template<class T>
struct remove_rvalue_ref {
    typedef typename std::conditional< 
        std::is_rvalue_reference<T>::value,
        typename std::remove_reference<T>::type,
        T
    >::type type;
};

#endif // TYPE_UTILITY_H
