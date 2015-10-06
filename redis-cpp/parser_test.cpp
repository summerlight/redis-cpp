#include <vector>
#include <string>
#include <iterator>
#include <algorithm>
#include <numeric>
#include <cstdint>
#include <cinttypes>
#include <cstddef>
#include <cassert>

#include "reply.h"
#include "writer.h"
#include "redis_test.h"

namespace redis_test
{

using std::begin;
using std::end;

// utility functions to help test routine
reply make_status_reply(const char reply_str[])
{
	reply r;
	r.t = reply::status;
	r.str = reply_str;
	return std::move(r);
}

reply make_error_reply(const char reply_str[])
{
	reply r;
	r.t = reply::error;
	r.str = reply_str;
	return std::move(r);
}

reply make_int_reply(const int32_t num)
{
	reply r;
	r.t = reply::integer_type;
	r.num = num;
	return std::move(r);
}

reply make_bulk_reply(size_t size)
{
	reply r;
	r.t = reply::bulk_type;
	r.bulk = std::vector<char>(size);
	std::generate(begin(r.bulk), end(r.bulk), []{ return rand(); });
	return std::move(r);
}

reply make_bulk_reply(const char str[])
{
	reply r;
	r.t = reply::bulk_type;
	r.bulk = std::vector<char>(str, str+strlen(str));
	return std::move(r);
}


reply make_multi_bulk_reply(std::vector<size_t> size_list)
{
	reply r;
	r.t = reply::multi_bulk_type;
	r.multi_bulk.reserve(size_list.size());

	for (auto i = begin(size_list), e = end(size_list); i != e; ++i) {
		if (*i == size_t(-1)) {
			r.multi_bulk.emplace_back(nullptr);
		} else {
			r.multi_bulk.emplace_back(new reply(make_bulk_reply(*i)));
		}
	}

	return std::move(r);
}

reply make_recursive_reply(size_t depth)
{
	reply r;
	auto size = rand() % std::min(std::max(depth, 1u), 5u);

	r.t = reply::multi_bulk_type;
	r.multi_bulk.reserve(size);

	for (size_t i = 0; i < size; i++) {
		auto random = rand();
		if (random % 10 > 7 && depth > 0) {
			r.multi_bulk.emplace_back(new reply(make_recursive_reply(depth - 1)));			
		} else if (random % 10 > 4) {
			r.multi_bulk.emplace_back(new reply(make_bulk_reply(rand() % 200)));
		} else if (random % 10 > 1) {
			r.multi_bulk.emplace_back(new reply(make_int_reply(rand())));
		} else {
			r.multi_bulk.emplace_back(nullptr);
		}
	}
	return std::move(r);
}


// reply serialization routine and its test routine only for parser testing
void serialize(const reply* r, mock_stream& input)
{
	char buffer[256];
	if (r == nullptr) {
		snprintf(buffer, sizeof(buffer), "$-1\r\n");
		input.more_input(buffer);
		return;
	}
	switch (r->t) {
	case reply::status:
		snprintf(buffer, sizeof(buffer), "+%s\r\n", r->str.c_str());
		input.more_input(buffer);
		break;
	case reply::error:
		snprintf(buffer, sizeof(buffer), "-%s\r\n", r->str.c_str());
		input.more_input(buffer);
		break;
	case reply::integer_type:
		snprintf(buffer, sizeof(buffer), ":%" PRId64 "\r\n", r->num);
		input.more_input(buffer);
		break;
	case reply::bulk_type:
		snprintf(buffer, sizeof(buffer), "$%d\r\n", r->bulk.size());
		input.more_input(buffer);
		input.more_input(r->bulk);
		input.more_input("\r\n");
		break;
	case reply::multi_bulk_type:
		snprintf(buffer, sizeof(buffer), "*%d\r\n", r->multi_bulk.size());
		input.more_input(buffer);
		for (auto i = begin(r->multi_bulk), e = end(r->multi_bulk); i != e; ++i) {
			serialize(i->get(), input);
		}
		break;
	default: assert(false);
	}
}

template<size_t size>
void check_serialization_result(const char (&expected)[size], const reply* r)
{
	mock_stream i;
	serialize(r, i);
	assert(std::equal(begin(i.input_buffer), end(i.input_buffer), begin(expected), end(expected)));
}

void test_status_reply_serialization()
{
	// result of EVAL "return {ok='this is status reply'}" 0
	static const char expected[] = {
		'+', 't', 'h', 'i', 's', ' ', 'i', 's', ' ', 's',
		't', 'a', 't', 'u', 's', ' ', 'r', 'e', 'p', 'l',
		'y', '\r', '\n'
	};

	auto r = make_status_reply("this is status reply");
	check_serialization_result(expected, &r);
}

void test_error_reply_serialization()
{
	// result of EVAL "return {err='this is error reply'}" 0
	static const char expected[] = {
		'-', 't', 'h', 'i', 's', ' ', 'i', 's', ' ', 'e',
		'r', 'r', 'o', 'r', ' ', 'r', 'e', 'p', 'l', 'y',
		'\r', '\n'
	};

	auto r = make_error_reply("this is error reply");
	check_serialization_result(expected, &r);
}

void test_integer_reply_serialization()
{
	// result of EVAL "return 42" 0
	static const char expected[] = {
		':', '4', '2', '\r', '\n'
	};

	auto r = make_int_reply(42);
	check_serialization_result(expected, &r);
}

void test_nil_reply_serialization()
{
	// result of EVAL "return false" 0
	static const char expected[] = {
		'$', '-', '1', '\r', '\n'
	};
	check_serialization_result(expected, nullptr);
}

void test_bulk_reply_serialization()
{
	// result of EVAL "return 'this is bulk reply'" 0
	static const char expected[] = {
		'$', '1', '8', '\r', '\n', 't', 'h', 'i', 's', ' ',
		'i', 's', ' ', 'b', 'u', 'l', 'k', ' ', 'r', 'e',
		'p', 'l', 'y', '\r', '\n'
	};

	auto r = make_bulk_reply("this is bulk reply");
	check_serialization_result(expected, &r);
}

void test_multi_bulk_reply_serialization()
{
	// result of EVAL "return {'test', 'multi', 'bulk', 'reply', false}" 0
	static const char expected[] = {
		'*', '5', '\r', '\n', '$', '4', '\r', '\n', 't', 'e',
		's', 't', '\r', '\n', '$', '5', '\r', '\n', 'm', 'u',
		'l', 't', 'i', '\r', '\n', '$', '4', '\r', '\n', 'b',
		'u', 'l', 'k', '\r', '\n', '$', '5', '\r', '\n', 'r',
		'e', 'p', 'l', 'y', '\r', '\n', '$', '-', '1', '\r',
		'\n'
	};

	reply r;
	r.t = reply::multi_bulk_type;
	r.multi_bulk.reserve(5);
	r.multi_bulk.emplace_back(new reply(make_bulk_reply("test")));
	r.multi_bulk.emplace_back(new reply(make_bulk_reply("multi")));
	r.multi_bulk.emplace_back(new reply(make_bulk_reply("bulk")));
	r.multi_bulk.emplace_back(new reply(make_bulk_reply("reply")));
	r.multi_bulk.emplace_back(nullptr);

	check_serialization_result(expected, &r);
}

void test_recursive_reply_serialzation()
{
	// result of EVAL "return {'test', 0, {10, {'recursive reply', ''}, false}}" 0
	static const char expected[] = {
		'*', '3', '\r', '\n', '$', '4', '\r', '\n', 't', 'e',
		's', 't', '\r', '\n', ':', '0', '\r', '\n', '*', '3',
		'\r', '\n', ':', '1', '0', '\r', '\n', '*', '2', '\r',
		'\n', '$', '1', '5', '\r', '\n', 'r', 'e', 'c', 'u',
		'r', 's', 'i', 'v', 'e', ' ', 'r', 'e', 'p', 'l', 'y',
		'\r', '\n', '$', '0', '\r', '\n', '\r', '\n', '$', '-',
		'1', '\r', '\n'
	};

	reply r;
	r.t = reply::multi_bulk_type;
	r.multi_bulk.reserve(3);
	r.multi_bulk.emplace_back(new reply(make_bulk_reply("test")));
	r.multi_bulk.emplace_back(new reply(make_int_reply(0)));

	{
		reply r1;
		r1.t = reply::multi_bulk_type;
		r1.multi_bulk.reserve(3);
		r1.multi_bulk.emplace_back(new reply(make_int_reply(10)));
		{
			reply r2;
			r2.t = reply::multi_bulk_type;
			r2.multi_bulk.reserve(2);
			r2.multi_bulk.emplace_back(new reply(make_bulk_reply("recursive reply")));
			r2.multi_bulk.emplace_back(new reply(make_bulk_reply("")));
			r1.multi_bulk.emplace_back(new reply(std::move(r2)));
		}
		r1.multi_bulk.emplace_back(nullptr);
		r.multi_bulk.emplace_back(new reply(std::move(r1)));
	}

	check_serialization_result(expected, &r);
}

void test_serialize_routine()
{
	test_status_reply_serialization();
	test_error_reply_serialization();
	test_integer_reply_serialization();
	test_nil_reply_serialization();
	test_bulk_reply_serialization();
	test_multi_bulk_reply_serialization();
	test_recursive_reply_serialzation();
}


// actual parser testing routine and its helper class


void test_reply(reply&& r)
{
	mock_stream in;
	reply_builder b;
	serialize(&r, in);
	auto ec = redis::parse(in, b);
	assert(!ec && r == *(b.root));
}

void test_error_reply(reply&& r)
{
	mock_stream in;
	reply_builder b;
	serialize(&r, in);
	auto ec = redis::parse(in, b);
	assert(ec == redis::error::error_reply && r == *(b.root));
}

void test_status_reply()
{
	test_reply(make_status_reply("OK"));
	test_error_reply(make_error_reply("ERR"));
	test_reply(make_status_reply("PONG"));
	test_reply(make_status_reply("QUEUED"));
	test_error_reply(make_error_reply("ERR Operation against a key holding the wrong kind of value"));
	test_error_reply(make_error_reply("ERR no such key"));
	test_error_reply(make_error_reply("ERR syntax error"));
	test_error_reply(make_error_reply("ERR source and destination objects are the same"));
	test_error_reply(make_error_reply("ERR index out of range"));
	test_error_reply(make_error_reply("LOADING Redis is loading the dataset in memory"));
}

void test_integer_reply()
{
	for (int i = 0; i < 1000; i++) {
		test_reply(make_int_reply(i));
	}

	for (int i = 0; i < 1000; i++) {
		test_reply(make_int_reply(rand() << 15 | rand()));
	}
}

void test_bulk_reply()
{
	for (auto i = 0; i < 1000; i++) {
		test_reply(make_bulk_reply(rand() % 950 + 50));
	}
}

void test_nil_reply()
{
	mock_stream in;
	reply_builder b;
	serialize(nullptr, in); // nil reply
	auto ec = redis::parse(in, b);
	assert(!ec && (b.root.get() == nullptr));
}

void test_multi_bulk_reply()
{
	for (auto i = 0; i < 1000; i++) {
		std::vector<size_t> data(rand() % 10);
		std::generate(begin(data), end(data), []()->size_t {
			const int max_size = 50;
			auto size = rand() % (max_size+1);
			return size == max_size ? size_t(-1) : size;
		});

		test_reply(make_multi_bulk_reply(data));
	}
}

void test_recursive_reply()
{
	for (auto i = 0; i < 1000; i++) {
		test_reply(make_recursive_reply(rand() % 10));
	}
}

void test_handler_error()
{
	struct handler_error : public redis::reply_handler_base
	{
		handler_error() : bulk_count(0) {}

		virtual bool on_multi_bulk_begin(size_t count) override
		{
			return true;
		}

		virtual bool on_bulk(redis::const_buffer_view data) override
		{	
			return bulk_count++ < 3;
		}

		virtual bool on_integer(int64_t value) override
		{
			return value > 100;
		}
		int32_t bulk_count;
	} handler;

	{
		auto r = make_int_reply(50);
		mock_stream in;
		serialize(&r, in);
		auto ec = redis::parse(in, handler);
		assert(ec == redis::error::handler_error);
	}
	
	{
		auto r = make_int_reply(150);
		mock_stream in;
		serialize(&r, in);
		auto ec = redis::parse(in, handler);
		assert(!ec);
	}

	{
		std::vector<size_t> data(10);
		std::generate(begin(data), end(data), []() {
			return 10;
		});

		auto r = make_multi_bulk_reply(data);
		mock_stream in;
		serialize(&r, in);
		auto ec = redis::parse(in, handler);
		assert(ec == redis::error::handler_error && handler.bulk_count == 4);
	}
}

void test_ill_formed_reply()
{
	{
		mock_stream in;
		in.more_input(":42a\r\n");
		redis::integer_reply handler;
		auto ec = redis::parse(in, handler);
		assert(ec == redis::error::ill_formed_reply && handler.result == -1);
	}

	{
		mock_stream in;
		in.more_input("a");
		redis::integer_reply handler;
		auto ec = redis::parse(in, handler);
		assert(ec == redis::error::ill_formed_reply && handler.result == -1);
	}
}

void test_unexpected_end_of_reply()
{
	{
		mock_stream in;
		in.more_input(":42\r");
		redis::integer_reply handler;
		auto ec = redis::parse(in, handler);
		assert(ec == redis::error::stream_error && handler.result == -1);
	}
	
	{
		mock_stream in;
		in.more_input(":42\r");
		redis::integer_reply handler;
		auto ec = redis::parse(in, handler);
		assert(ec == redis::error::stream_error && handler.result == -1);
	}

	{
		mock_stream in;
		in.more_input(":");
		redis::integer_reply handler;
		auto ec = redis::parse(in, handler);
		assert(ec == redis::error::stream_error && handler.result == -1);
	}

	{
		mock_stream in;
		in.more_input("$18\r\nthis is bulk r");
		redis::bulk_reply handler;
		auto ec = redis::parse(in, handler);
		assert(ec == redis::error::stream_error);
	}

	{
		mock_stream in;
		in.more_input("*5\r\n$4\r\ntest\r\n$5\r\nmulti\r\n$5\r\nreply\r\n$-1\r\n");
		redis::multi_bulk_reply handler;
		auto ec = redis::parse(in, handler);
		assert(ec == redis::error::stream_error);
	}
}

void test_parser()
{
	test_serialize_routine();

	test_status_reply();
	test_integer_reply();
	test_bulk_reply();
	test_nil_reply();
	test_multi_bulk_reply();
	test_recursive_reply();

	test_handler_error();
	test_ill_formed_reply();
	test_unexpected_end_of_reply();
}

} // the end of namespace "redis_test"
