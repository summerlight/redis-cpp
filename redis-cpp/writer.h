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

} // the end of namespace "redis::detail"

// header for request
inline bool write_header(stream& output, size_t size)
{
	return output.write('*') &&
		detail::write_integer(output, size) &&
		detail::write_newline(output);
}

template<typename T, typename enable = void>
struct writer_type_traits;

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

// writer wrapper functions
template<typename T>
inline typename std::enable_if<
	writer_type_traits<typename std::decay<T>::type>::static_count,
	size_t
>::type
	count_element(T&& value)
{
	return writer_type_traits<typename std::decay<T>::type>::count;
}

template<typename T>
inline typename std::enable_if<
	!writer_type_traits<typename std::decay<T>::type>::static_count,
	size_t
>::type
	count_element(T&& value)
{
	return writer_type_traits<typename std::decay<T>::type>::count(std::forward<T>(value));
}

template<typename T>
bool write_element(stream& output, T&& value)
{
	return writer_type_traits<typename std::decay<T>::type>::write(output, std::forward<T>(value));
}

// vs 2010 doesn't support variadic template - using code generator as a workaround
// begin of auto-generated code
template <
	typename T1,
	typename T2
>
size_t count_element(T1&& v1, T2&& v2)
{
	return count_element(fwd(1)) + count_element(fwd(2));
}

template <
	typename T1,
	typename T2,
	typename T3
>
size_t count_element(T1&& v1, T2&& v2, T3&& v3)
{
	return count_element(fwd(1), fwd(2)) + count_element(fwd(3));
}

template <
	typename T1,
	typename T2,
	typename T3,
	typename T4
>
size_t count_element(T1&& v1, T2&& v2, T3&& v3, T4&& v4)
{
	return count_element(fwd(1), fwd(2), fwd(3)) + count_element(fwd(4));
}

template <
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5
>
size_t count_element(T1&& v1, T2&& v2, T3&& v3, T4&& v4, T5&& v5)
{
	return count_element(fwd(1), fwd(2), fwd(3), fwd(4)) + count_element(fwd(5));
}

template <
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6
>
size_t count_element(T1&& v1, T2&& v2, T3&& v3, T4&& v4, T5&& v5, T6&& v6)
{
	return count_element(fwd(1), fwd(2), fwd(3), fwd(4), fwd(5)) + count_element(fwd(6));
}

template <
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6,
	typename T7
>
size_t count_element(T1&& v1, T2&& v2, T3&& v3, T4&& v4, T5&& v5, T6&& v6, T7&& v7)
{
	return count_element(fwd(1), fwd(2), fwd(3), fwd(4), fwd(5), fwd(6)) + count_element(fwd(7));
}

template <
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6,
	typename T7,
	typename T8
>
size_t count_element(T1&& v1, T2&& v2, T3&& v3, T4&& v4, T5&& v5, T6&& v6, T7&& v7, T8&& v8)
{
	return count_element(fwd(1), fwd(2), fwd(3), fwd(4), fwd(5), fwd(6), fwd(7)) + count_element(fwd(8));
}

template <
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6,
	typename T7,
	typename T8,
	typename T9
>
size_t count_element(T1&& v1, T2&& v2, T3&& v3, T4&& v4, T5&& v5, T6&& v6, T7&& v7, T8&& v8, T9&& v9)
{
	return count_element(fwd(1), fwd(2), fwd(3), fwd(4), fwd(5), fwd(6), fwd(7), fwd(8)) + count_element(fwd(9));
}

template <
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6,
	typename T7,
	typename T8,
	typename T9,
	typename T10
>
size_t count_element(T1&& v1, T2&& v2, T3&& v3, T4&& v4, T5&& v5, T6&& v6, T7&& v7, T8&& v8, T9&& v9, T10&& v10)
{
	return count_element(fwd(1), fwd(2), fwd(3), fwd(4), fwd(5), fwd(6), fwd(7), fwd(8), fwd(9)) + count_element(fwd(10));
}

template <
	typename T1,
	typename T2
>
bool write_element(stream& output, T1&& v1, T2&& v2)
{
	return write_element(output, fwd(1)) && write_element(output, fwd(2));
}

template <
	typename T1,
	typename T2,
	typename T3
>
bool write_element(stream& output, T1&& v1, T2&& v2, T3&& v3)
{
	return write_element(output, fwd(1), fwd(2)) && write_element(output, fwd(3));
}

template <
	typename T1,
	typename T2,
	typename T3,
	typename T4
>
bool write_element(stream& output, T1&& v1, T2&& v2, T3&& v3, T4&& v4)
{
	return write_element(output, fwd(1), fwd(2), fwd(3)) && write_element(output, fwd(4));
}

template <
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5
>
bool write_element(stream& output, T1&& v1, T2&& v2, T3&& v3, T4&& v4, T5&& v5)
{
	return write_element(output, fwd(1), fwd(2), fwd(3), fwd(4)) && write_element(output, fwd(5));
}

template <
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6
>
bool write_element(stream& output, T1&& v1, T2&& v2, T3&& v3, T4&& v4, T5&& v5, T6&& v6)
{
	return write_element(output, fwd(1), fwd(2), fwd(3), fwd(4), fwd(5)) && write_element(output, fwd(6));
}

template <
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6,
	typename T7
>
bool write_element(stream& output, T1&& v1, T2&& v2, T3&& v3, T4&& v4, T5&& v5, T6&& v6, T7&& v7)
{
	return write_element(output, fwd(1), fwd(2), fwd(3), fwd(4), fwd(5), fwd(6)) && write_element(output, fwd(7));
}

template <
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6,
	typename T7,
	typename T8
>
bool write_element(stream& output, T1&& v1, T2&& v2, T3&& v3, T4&& v4, T5&& v5, T6&& v6, T7&& v7, T8&& v8)
{
	return write_element(output, fwd(1), fwd(2), fwd(3), fwd(4), fwd(5), fwd(6), fwd(7)) && write_element(output, fwd(8));
}

template <
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6,
	typename T7,
	typename T8,
	typename T9
>
bool write_element(stream& output, T1&& v1, T2&& v2, T3&& v3, T4&& v4, T5&& v5, T6&& v6, T7&& v7, T8&& v8, T9&& v9)
{
	return write_element(output, fwd(1), fwd(2), fwd(3), fwd(4), fwd(5), fwd(6), fwd(7), fwd(8)) && write_element(output, fwd(9));
}

template <
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6,
	typename T7,
	typename T8,
	typename T9,
	typename T10
>
bool write_element(stream& output, T1&& v1, T2&& v2, T3&& v3, T4&& v4, T5&& v5, T6&& v6, T7&& v7, T8&& v8, T9&& v9, T10&& v10)
{
	return write_element(output, fwd(1), fwd(2), fwd(3), fwd(4), fwd(5), fwd(6), fwd(7), fwd(8), fwd(9)) && write_element(output, fwd(10));
}

template <
	typename T1
>
std::error_code format_command(stream& output, T1&& v1)
{
	return write_header(output, count_element(fwd(1))) && 
		write_element(output, fwd(1)) ? std::error_code() : error::stream_error;
}

template <
	typename T1,
	typename T2
>
std::error_code format_command(stream& output, T1&& v1, T2&& v2)
{
	return write_header(output, count_element(fwd(1), fwd(2))) && 
		write_element(output, fwd(1), fwd(2)) ? std::error_code() : error::stream_error;
}

template <
	typename T1,
	typename T2,
	typename T3
>
std::error_code format_command(stream& output, T1&& v1, T2&& v2, T3&& v3)
{
	return write_header(output, count_element(fwd(1), fwd(2), fwd(3))) && 
		write_element(output, fwd(1), fwd(2), fwd(3)) ? std::error_code() : error::stream_error;
}

template <
	typename T1,
	typename T2,
	typename T3,
	typename T4
>
std::error_code format_command(stream& output, T1&& v1, T2&& v2, T3&& v3, T4&& v4)
{
	return write_header(output, count_element(fwd(1), fwd(2), fwd(3), fwd(4))) && 
		write_element(output, fwd(1), fwd(2), fwd(3), fwd(4)) ? std::error_code() : error::stream_error;
}

template <
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5
>
std::error_code format_command(stream& output, T1&& v1, T2&& v2, T3&& v3, T4&& v4, T5&& v5)
{
	return write_header(output, count_element(fwd(1), fwd(2), fwd(3), fwd(4), fwd(5))) && 
		write_element(output, fwd(1), fwd(2), fwd(3), fwd(4), fwd(5)) ? std::error_code() : error::stream_error;
}

template <
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6
>
std::error_code format_command(stream& output, T1&& v1, T2&& v2, T3&& v3, T4&& v4, T5&& v5, T6&& v6)
{
	return write_header(output, count_element(fwd(1), fwd(2), fwd(3), fwd(4), fwd(5), fwd(6))) && 
		write_element(output, fwd(1), fwd(2), fwd(3), fwd(4), fwd(5), fwd(6)) ? std::error_code() : error::stream_error;
}

template <
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6,
	typename T7
>
std::error_code format_command(stream& output, T1&& v1, T2&& v2, T3&& v3, T4&& v4, T5&& v5, T6&& v6, T7&& v7)
{
	return write_header(output, count_element(fwd(1), fwd(2), fwd(3), fwd(4), fwd(5), fwd(6), fwd(7))) && 
		write_element(output, fwd(1), fwd(2), fwd(3), fwd(4), fwd(5), fwd(6), fwd(7)) ? std::error_code() : error::stream_error;
}

template <
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6,
	typename T7,
	typename T8
>
std::error_code format_command(stream& output, T1&& v1, T2&& v2, T3&& v3, T4&& v4, T5&& v5, T6&& v6, T7&& v7, T8&& v8)
{
	return write_header(output, count_element(fwd(1), fwd(2), fwd(3), fwd(4), fwd(5), fwd(6), fwd(7), fwd(8))) && 
		write_element(output, fwd(1), fwd(2), fwd(3), fwd(4), fwd(5), fwd(6), fwd(7), fwd(8)) ? std::error_code() : error::stream_error;
}

template <
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6,
	typename T7,
	typename T8,
	typename T9
>
std::error_code format_command(stream& output, T1&& v1, T2&& v2, T3&& v3, T4&& v4, T5&& v5, T6&& v6, T7&& v7, T8&& v8, T9&& v9)
{
	return write_header(output, count_element(fwd(1), fwd(2), fwd(3), fwd(4), fwd(5), fwd(6), fwd(7), fwd(8), fwd(9))) && 
		write_element(output, fwd(1), fwd(2), fwd(3), fwd(4), fwd(5), fwd(6), fwd(7), fwd(8), fwd(9)) ? std::error_code() : error::stream_error;
}

template <
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6,
	typename T7,
	typename T8,
	typename T9,
	typename T10
>
std::error_code format_command(stream& output, T1&& v1, T2&& v2, T3&& v3, T4&& v4, T5&& v5, T6&& v6, T7&& v7, T8&& v8, T9&& v9, T10&& v10)
{
	return write_header(output, count_element(fwd(1), fwd(2), fwd(3), fwd(4), fwd(5), fwd(6), fwd(7), fwd(8), fwd(9), fwd(10))) && 
		write_element(output, fwd(1), fwd(2), fwd(3), fwd(4), fwd(5), fwd(6), fwd(7), fwd(8), fwd(9), fwd(10)) ? std::error_code() : error::stream_error;
}
// end of auto-generated code

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
