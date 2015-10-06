#ifndef REDIS_WRITER_TYPE_TRAITS
#define REDIS_WRITER_TYPE_TRAITS

#include <string>
#include <vector>
#include <utility>
#include <type_traits>
#include <cstring>

#include "writer.h"

namespace redis
{

// element trait classes
template<>
struct writer_type_traits<std::vector<char>>
{
	static const bool static_count = true;
	static const size_t count = 1;

	inline static bool write(stream& output, const std::vector<char>& value) {
		return detail::write_bulk_element(output, const_buffer_view(value.data(), value.size()));
	}
};

template<>
struct writer_type_traits<char*>
{
	static const bool static_count = true;
	static const size_t count = 1;

	inline static bool write(stream& output, char* str)
	{
		return detail::write_bulk_element(output, const_buffer_view(str, strlen(str)));
	}
};

template<>
struct writer_type_traits<const char*>
{
	static const bool static_count = true;
	static const size_t count = 1;

	inline static bool write(stream& output, const char* str)
	{
		return detail::write_bulk_element(output, const_buffer_view(str, strlen(str)));
	}
};

template<>
struct writer_type_traits<const wchar_t*>
{
	static const bool static_count = true;
	static const size_t count = 1;

	inline static bool write(stream& output, const wchar_t* str)
	{
		typedef redis::const_buffer_view::pointer ptr_type;
		return detail::write_bulk_element(output, const_buffer_view(
			static_cast<ptr_type>(static_cast<const void*>(str)),
			wcslen(str)*sizeof(wchar_t)
		));
	}
};

template<>
struct writer_type_traits<wchar_t*>
{
	static const bool static_count = true;
	static const size_t count = 1;

	inline static bool write(stream& output, wchar_t* str)
	{
		return writer_type_traits<const wchar_t*>::write(output, str);
	}
};

template<>
struct writer_type_traits<std::string>
{
	static const bool static_count = true;
	static const size_t count = 1;

	inline static bool write(stream& output, const std::string& value) {
		return detail::write_bulk_element(output, const_buffer_view(value.c_str(), value.size()));
	}
};

template<>
struct writer_type_traits<std::wstring>
{
	static const bool static_count = true;
	static const size_t count = 1;

	inline static bool write(stream& output, const std::wstring& value) {
		typedef redis::const_buffer_view::pointer ptr_type;
		return detail::write_bulk_element(output, redis::const_buffer_view(
			static_cast<ptr_type>(static_cast<const void*>(value.c_str())),
			value.size() * sizeof(*ptr_type())
		));
	}
};

template<>
struct writer_type_traits<buffer_view>
{
	static const bool static_count = true;
	static const size_t count = 1;
	inline static bool write(stream& output, buffer_view buf)
	{
		return detail::write_bulk_element(output, const_buffer_view(buf));
	}
};

template<>
struct writer_type_traits<const_buffer_view>
{
	static const bool static_count = true;
	static const size_t count = 1;
	inline static bool write(stream& output, const_buffer_view buf)
	{
		return detail::write_bulk_element(output, buf);
	}
};

template<typename T>
struct writer_type_traits<
	T,
	typename std::enable_if<std::is_integral<T>::value>::type
>
{
	static const bool static_count = true;
	static const size_t count = 1;
	inline static bool write(stream& output, T value)
	{
		char buffer[24];
		auto result = detail::write_int_on_buf(buffer_view(begin(buffer), end(buffer)), value);
		return detail::write_bulk_element(output, result);
	}
};

template<typename T1, typename T2>
struct writer_type_traits<std::pair<T1, T2>>
{
	static const bool static_count = true;
	static const size_t count = 
		writer_type_traits<T1>::count +
		writer_type_traits<T2>::count;

	inline static bool write(stream& output, const std::pair<T1, T2>& value)
	{
		return write_element(output, value.first) && write_element(output, value.second);
	}
};

template<typename T>
struct writer_type_traits<
	T,
	typename std::enable_if<is_iterable<T>::value>::type
>
{
	static const bool static_count = false;

	inline static size_t count(const T& value)
	{
		auto&& i = begin(value);
		auto&& e = end(value);

		typedef typename std::decay<decltype(*i)>::type value_type;
		static_assert(writer_type_traits<value_type>::static_count, "do not support formating for recursive container.");
		return std::distance(i, e) * writer_type_traits<value_type>::count;
	}

	inline static bool write(stream& output, const T& value)
	{
		auto&& i = begin(value);
		auto&& e = end(value);

		typedef typename std::decay<decltype(*i)>::type value_type;
		static_assert(writer_type_traits<value_type>::static_count, "do not support formating for recursive container.");
		for(; i != e; ++i) {
			if (!write_element(output, *i)) {
				return false;
			}
		}
		return true;
	}
};

template<typename T1>
struct writer_type_traits<redis::detail::opt<T1>>
{
	static const bool static_count = false;

	inline static size_t count(const redis::detail::opt<T1>& value)
	{
		return value.condition ? count_element(value.v1) : 0;
	}

	inline static bool write(stream& output, const redis::detail::opt<T1>& value)
	{
		return value.condition ? write_element(output, value.v1) : true;
	}
};

template<typename T1, typename T2>
struct writer_type_traits<redis::detail::opt<T1, T2>>
{
	static const bool static_count = false;

	inline static size_t count(const redis::detail::opt<T1, T2>& value)
	{
		return value.condition ? count_element(value.v1, value.v2) : 0;
	}

	inline static bool write(stream& output, const redis::detail::opt<T1, T2>& value)
	{
		return value.condition ? write_element(output, value.v1, value.v2) : true;
	}
};

template<typename T1, typename T2, typename T3>
struct writer_type_traits<redis::detail::opt<T1, T2, T3>>
{
	static const bool static_count = false;

	inline static size_t count(const redis::detail::opt<T1, T2, T3>& value)
	{
		return value.condition ? count_element(value.v1, value.v2, value.v3) : 0;
	}

	inline static bool write(stream& output, const redis::detail::opt<T1, T2, T3>& value)
	{
		return value.condition ? write_element(output, value.v1, value.v2, value.v3) : true;
	}
};

} // the end of namespace "redis"

#endif // REDIS_WRITER_TYPE_TRAITS
