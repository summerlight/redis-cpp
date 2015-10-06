#ifndef TYPE_UTILITY_H
#define TYPE_UTILITY_H

#include <utility>
#include <type_traits>
#include <iterator>

template<int> void begin();
template<int> void end();

template <typename T>
typename std::add_rvalue_reference<T>::type declval(); // vs2010 does not support std::declval - workaround

template <bool b>
struct error_if_false;

template <>
struct error_if_false<true>
{
};

template<typename U>
struct is_input_iterator
{
	static const bool value = std::is_base_of<
		std::input_iterator_tag,
		typename std::iterator_traits<U>::iterator_category
	>::value;
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
			is_input_iterator<decltype(begin(declval<U>()))>::value &&
			is_input_iterator<decltype(end(declval<U>()))>::value &&
			std::is_same<
				decltype(begin(declval<U>())),
				decltype(end(declval<U>()))
			>::value
		>(),
		yes()
	);

	template<typename>
	static no check(...);

public:
	static const bool value = (sizeof(check<typename std::decay<T>::type>(nullptr)) == sizeof(yes));
};

template<class T>
struct remove_rvalue_ref {
	typedef typename std::conditional< 
		std::is_rvalue_reference<T>::value,
		typename std::remove_reference<T>::type,
		T
	>::type type;
};

#endif // TYPE_UTILITY_H
