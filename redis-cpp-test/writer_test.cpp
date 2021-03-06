#include "redis_test.h"
#include "writer_type_traits.h"

#include <vector>
#include <list>
#include <string>
#include <iterator>
#include <utility>
#include <cassert>
#include <cstdio>
#include <cinttypes>

#include <catch.hpp>

namespace redis_test
{
using std::begin;
using std::end;

// element count function test
TEST_CASE("element_count", "[writer]")
{
    // scalar type test
    int a = 0;
    std::string b = "test";
    std::wstring c = L"test";
    char* d = "";

    REQUIRE(redis::count_element(a) == 1);
    REQUIRE(redis::count_element(b) == 1);
    REQUIRE(redis::count_element(c) == 1);
    REQUIRE(redis::count_element(d) == 1);
    REQUIRE(redis::count_element(0) == 1);
    REQUIRE(redis::count_element(std::string("")) == 1);
    REQUIRE(redis::count_element(std::wstring(L"")) == 1);
    REQUIRE(redis::count_element("") == 1);

    // pair type test
    std::pair<int, std::wstring> e;
    REQUIRE(redis::count_element(e) == 2);
    REQUIRE(redis::count_element(std::make_pair(0, "")) == 2);

    // optional type test
    REQUIRE(redis::count_element(redis::optional(true, a, "test", std::make_pair(10, 10))) == 4);
    REQUIRE(redis::count_element(redis::optional(false, b, "test", std::make_pair(10, 10))) == 0);
    REQUIRE(!b.empty());

    // variadic argument test
    REQUIRE(redis::count_element(0) == 1);
    REQUIRE(redis::count_element(0, 0) == 2);
    REQUIRE(redis::count_element(0, 0, 0) == 3);
    REQUIRE(redis::count_element(0, 0, 0, 0) == 4);
    REQUIRE(redis::count_element(0, 0, 0, 0, 0) == 5);
    REQUIRE(redis::count_element(0, 0, 0, 0, 0, 0) == 6);
    REQUIRE(redis::count_element(0, 0, 0, 0, 0, 0, 0) == 7);
    REQUIRE(redis::count_element(0, 0, 0, 0, 0, 0, 0, 0) == 8);
    REQUIRE(redis::count_element(0, 0, 0, 0, 0, 0, 0, 0, 0) == 9);
    REQUIRE(redis::count_element(0, 0, 0, 0, 0, 0, 0, 0, 0, 0) == 10);
}

TEST_CASE("container_count", "[writer]")
{
    {
        // empty
        std::vector<int> vec;
        REQUIRE(redis::count_element(vec) == 0);
    }

    {
        // l-value
        std::vector<int> vec;
        vec.push_back(1);
        vec.push_back(2);
        vec.push_back(3);
        REQUIRE(redis::count_element(vec) == 3);
    }

    {
        // const l-value
        const std::vector<std::string> vec(3);
        REQUIRE(redis::count_element(vec) == 3);
    }

    {
        // r-value
        REQUIRE(redis::count_element(std::vector<std::wstring>(3)) == 3u);
    }

    {
        // pair vector
        std::vector<std::pair<int, std::string>> vec;
        vec.push_back(std::make_pair(0, "0"));
        vec.push_back(std::make_pair(1, "1"));
        vec.push_back(std::make_pair(2, "2"));
        REQUIRE(redis::count_element(vec) == 6u);
    }

    {
        // list
        std::list<int> list;
        list.push_back(0);
        list.push_back(1);
        list.push_back(2);
        list.push_back(3);
        REQUIRE(redis::count_element(list) == 4u);
    }

    {
        // optional type with container test
        std::vector<int> vec;
        vec.push_back(1);
        vec.push_back(2);
        REQUIRE(redis::count_element(redis::optional(true, vec)) == 2);
        REQUIRE(vec.size() == 2); // optional value for l-value should not move container - only for r-value
    }
}

void write_integer_test(int64_t value)
{
    mock_stream output;
    redis::detail::write_integer(output, value);
    char buffer[24];
    snprintf(buffer, 24, "%" PRId64, value);
    REQUIRE(check_equal(buffer, output));
}

TEST_CASE("writer_helper_function", "[writer]")
{
    for (int i = 0; i < 100; i++) {
        write_integer_test(uniform_random<int64_t>());
    }
    write_integer_test(std::numeric_limits<int64_t>::max());
    write_integer_test(std::numeric_limits<int64_t>::min());
    write_integer_test(0);

    {
        mock_stream output;
        redis::detail::write_newline(output);
        REQUIRE(check_equal("\r\n", output));
    }

    {
        mock_stream output;
        const char test_data[] = "this is test";
        redis::detail::write_bulk_element(output, redis::const_buffer_view(begin(test_data), end(test_data)-1));
        REQUIRE(check_equal("$12\r\nthis is test\r\n", output));
    }

    {
        mock_stream output;
        redis::write_header(output, 10);
        REQUIRE(check_equal("*10\r\n", output));
    }
}

template<typename T>
void write_element_test(T&& value, const char expected[])
{
    mock_stream output;
    redis::write_element(output, std::forward<T>(value));
    REQUIRE(check_equal(expected, output));
}

TEST_CASE("write_element_for_each_type", "[writer]")
{
    write_element_test(10, "$2\r\n10\r\n");
    write_element_test("test", "$4\r\ntest\r\n");
    write_element_test(std::make_pair(1, 2), "$1\r\n1\r\n$1\r\n2\r\n");
    write_element_test(std::string("test"), "$4\r\ntest\r\n");
    {
        std::vector<char> vec;
        vec.push_back('a');
        vec.push_back('b');
        vec.push_back('c');
        write_element_test(vec, "$3\r\nabc\r\n");
    }
    {
        std::vector<int> vec;
        vec.push_back(1);
        vec.push_back(2);
        vec.push_back(3);
        write_element_test(vec, "$1\r\n1\r\n$1\r\n2\r\n$1\r\n3\r\n");
    }

    write_element_test(redis::optional(true, 1, "test"), "$1\r\n1\r\n$4\r\ntest\r\n");

    {
        const char data[] = "1234";
        write_element_test(redis::const_buffer_view(begin(data), end(data)-1), "$4\r\n1234\r\n");
    }
}

TEST_CASE("write_variadic_element", "[writer]")
{
    {
        mock_stream output;
        redis::write_element(output, 0);
        REQUIRE(check_equal("$1\r\n0\r\n", output));
    }
    
    {
        mock_stream output;
        redis::write_element(output, 0, 0);
        REQUIRE(check_equal("$1\r\n0\r\n$1\r\n0\r\n", output));
    }

    {
        mock_stream output;
        redis::write_element(output, 0, 0, 0);
        REQUIRE(check_equal("$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n", output));
    }

    {
        mock_stream output;
        redis::write_element(output, 0, 0, 0, 0);
        REQUIRE(check_equal("$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n", output));
    }

    {
        mock_stream output;
        redis::write_element(output, 0, 0, 0, 0, 0);
        REQUIRE(check_equal("$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n", output));
    }

    {
        mock_stream output;
        redis::write_element(output, 0, 0, 0, 0, 0, 0);
        REQUIRE(check_equal("$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n", output));
    }

    {
        mock_stream output;
        redis::write_element(output, 0, 0, 0, 0, 0, 0, 0);
        REQUIRE(check_equal("$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n", output));
    }

    {
        mock_stream output;
        redis::write_element(output, 0, 0, 0, 0, 0, 0, 0, 0);
        REQUIRE(check_equal("$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n", output));
    }

    {
        mock_stream output;
        redis::write_element(output, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        REQUIRE(check_equal("$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n", output));
    }

    {
        mock_stream output;
        redis::write_element(output, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        REQUIRE(check_equal("$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n$1\r\n0\r\n", output));
    }
}

} // namespace "redis_test"
