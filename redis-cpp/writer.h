#ifndef REDIS_WRITER_H
#define REDIS_WRITER_H

#include <utility>
#include <type_traits>
#include <cstdint>

#include "redis_base.h"
#include "type_utility.h"

#define fwd(n) (std::forward<T##n>(v##n))

namespace redis
{

using std::begin;
using std::end;

template<typename T, typename enable = void>
struct writer_type_traits;

namespace detail
{

// detailed writer utility functions
template<typename integer_type>
inline typename std::enable_if<std::is_signed<integer_type>::value, uint64_t>::type
	get_remainder(integer_type value)
{
	return (value >= 0 ? value : -value);
}

template<typename integer_type>
inline typename std::enable_if<!std::is_signed<integer_type>::value, uint64_t>::type
	get_remainder(integer_type value)
{
	return value;
}

template<typename integer_type>
inline const_buffer_view write_int_on_buf(buffer_view buffer, integer_type value)
{
	uint64_t remainder = get_remainder(value);		
	size_t index = 0;

	do {
		buffer[index++] = remainder % 10 + '0';
		remainder /= 10;
	} while(remainder > 0);

	if (value < 0) {
		buffer[index++] = '-';
	}
	assert(index-1 < buffer.size());

	std::reverse(begin(buffer), begin(buffer)+index);

	return const_buffer_view(begin(buffer), index);
}

template<typename integer_type>
inline bool write_integer(stream& output, integer_type value)
{
	char buffer[24];
	return output.write(write_int_on_buf(buffer_view(begin(buffer), end(buffer)), value));
}

inline bool write_newline(stream& output)
{
	return output.write(redis::const_buffer_view(crlf, sizeof(crlf)));
}

inline bool write_bulk_element(stream& output, const_buffer_view buf)
{
	return output.write('$') &&
		write_integer(output, buf.size()) &&
		write_newline(output) &&
		output.write(buf) &&
		write_newline(output);
}

// writer wrapper functions
template<typename T>
inline typename std::enable_if<
	writer_type_traits<typename std::decay<T>::type>::static_count,
	size_t
>::type
count_element(const T& value)
{
	return writer_type_traits<typename std::decay<T>::type>::count;
}

template<typename T>
inline typename std::enable_if<
	!writer_type_traits<typename std::decay<T>::type>::static_count,
	size_t
>::type
count_element(const T& value)
{
	return writer_type_traits<typename std::decay<T>::type>::count(value);
}

template<typename T>
bool write_element(stream& output, const T& value)
{
	return writer_type_traits<typename std::decay<T>::type>::write(output, value);
}

} // the end of namespace "redis::detail"

// header for request
inline bool write_header(stream& output, size_t size)
{
	return output.write('*') &&
		detail::write_integer(output, size) &&
		detail::write_newline(output);
}

template<typename T, typename enable = void>
struct is_single_element_type;

template<typename T>
struct is_single_element_type<
	T,
	typename std::enable_if<writer_type_traits<T>::static_count>::type
>
{
	const static bool value = (writer_type_traits<T>::count == 1);
};

template<typename T>
struct is_single_element_type<
	T,
	typename std::enable_if<!writer_type_traits<T>::static_count>::type
>
{
	const static bool value = false;
};

// vs 2010 doesn't support variadic template - using code generator as a workaround
// begin of auto-generated code

inline size_t count_element()
{
	return 0u;
}

template<typename Head, typename... Remainder>
size_t count_element(const Head& h, const Remainder&... r)
{
	return detail::count_element(h) +
		count_element(r...);
}

inline bool write_element(stream& output)
{
	return true;
}

template<typename Head, typename... Remainder>
bool write_element(stream& output, const Head& h, const Remainder&... r)
{
	return detail::write_element(output, h) &&
		write_element(output, r...);
}

template<typename... Typelist>
std::error_code format_command(stream& output, const Typelist&... values)
{
	return write_header(output, count_element(values...)) &&
		write_element(output, values...) ? std::error_code() : error::stream_error;
}


namespace detail
{

template<typename T1, typename T2 = void, typename T3 = void>
struct opt
{
	opt(bool cond, T1 v1, T2 v2, T3 v3) : condition(cond), v1(std::forward<T1>(v1)), v2(std::forward<T2>(v2)), v3(std::forward<T3>(v3)) {}

	bool condition;
	typename remove_rvalue_ref<T1>::type v1;
	typename remove_rvalue_ref<T2>::type v2;
	typename remove_rvalue_ref<T3>::type v3;
};

template<typename T1>
struct opt<T1>
{
	opt(bool cond, T1 v1) : condition(cond), v1(std::forward<T1>(v1)) {}

	bool condition;
	typename remove_rvalue_ref<T1>::type v1;
};

template<typename T1, typename T2>
struct opt<T1, T2>
{
	opt(bool cond, T1 v1, T2 v2) : condition(cond), v1(std::forward<T1>(v1)), v2(std::forward<T2>(v2)) {}

	bool condition;
	typename remove_rvalue_ref<T1>::type v1;
	typename remove_rvalue_ref<T2>::type v2;
};

} // the end of namespace "redis::detail"

template<typename T1>
inline detail::opt<T1> optional(bool cond, T1&& v1)
{
	return detail::opt<T1>(cond, std::forward<T1>(v1));
}

template<typename T1, typename T2>
inline detail::opt<T1, T2> optional(bool cond, T1&& v1, T2&& v2)
{
	return detail::opt<T1, T2>(cond, std::forward<T1>(v1), std::forward<T2>(v2));
}

template<typename T1, typename T2, typename T3>
inline detail::opt<T1, T2, T3> optional(bool cond, T1&& v1, T2&& v2, T3&& v3)
{
	return detail::opt<T1, T2, T3>(cond, std::forward<T1>(v1), std::forward<T2>(v2), std::forward<T3>(v3));
}

} // the end of namespace "redis"

#undef fwd

#endif // REDIS_WRITER_H
