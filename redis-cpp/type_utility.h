#ifndef TYPE_UTILITY_H
#define TYPE_UTILITY_H

#include <utility>
#include <type_traits>
#include <iterator>

namespace detail {

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

template <typename R>
struct is_range : std::is_same<typename detail::begin_result<R>::type,
	typename detail::end_result<R>::type> {};

/*
template<typename T>
struct is_input_iterator
{
	using decay_t = typename std::decay<T>::type;
	using iterator_category_t = typename std::iterator_traits<decay_t>::iterator_category;
	static const bool value = std::is_base_of<
		std::input_iterator_tag,
		iterator_category_t
	>::value;
};

template <bool b>
struct error_if_false;

template <>
struct error_if_false<true>
{
};

template<typename T>
struct is_iterable
{
private:
	typedef char yes;
	typedef char (&no)[2];

	template<typename U>
	static auto check(U*) -> decltype(
		error_if_false<
			is_range<decltype(begin(std::declval<U>()))>::value &&
			is_range<decltype(end(std::declval<U>()))>::value &&
			std::is_same<
				decltype(begin(std::declval<U>())),
				decltype(end(std::declval<U>()))
			>::value
		>(),
		yes()
	);

	template<typename>
	static no check(...);

public:
	static const bool value = (sizeof(check<typename std::decay<T>::type>(nullptr)) == sizeof(yes));
};
*/

template<class T>
struct remove_rvalue_ref {
	typedef typename std::conditional< 
		std::is_rvalue_reference<T>::value,
		typename std::remove_reference<T>::type,
		T
	>::type type;
};

#endif // TYPE_UTILITY_H
