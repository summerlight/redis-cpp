#include "redis_test.h"

#include <vector>
#include <iterator>
#include <cstddef>
#include <cstdint>

#include "redis_base.h"
#include "command.h"

namespace redis_test
{

using std::begin;
using std::end;

void check_command_output(const redis::command& cmd, const char expected[])
{
	mock_stream output;
	auto ec = cmd.write_command(output);
	assert(!ec && check_equal(expected, output));
}
	
void test_key_command()
{
	redis::GET cmd;
	cmd.key = "this_is_key";
	check_command_output(cmd, "*2\r\n$3\r\nGET\r\n$11\r\nthis_is_key\r\n");
}

void test_generic_key_value_command()
{
	{
		redis::SET<int32_t> cmd;
		cmd.key = "this_is_key";
		cmd.value = 10;
		check_command_output(cmd, "*3\r\n$3\r\nSET\r\n$11\r\nthis_is_key\r\n$2\r\n10\r\n");
	}
	
	{
		redis::SET<std::string> cmd;
		cmd.key = "this_is_key";
		cmd.value = "this_is_value";
		check_command_output(cmd, "*3\r\n$3\r\nSET\r\n$11\r\nthis_is_key\r\n$13\r\nthis_is_value\r\n");
	}
	
	{
		redis::SET<const char*> cmd;
		cmd.key = "this_is_key";
		cmd.value = "this_is_value";
		check_command_output(cmd, "*3\r\n$3\r\nSET\r\n$11\r\nthis_is_key\r\n$13\r\nthis_is_value\r\n");
	}
	
	{
		redis::SET<std::vector<char>> cmd;
		cmd.key = "this_is_key";
		const char bulk[] = "this_is_value";
		cmd.value.insert(end(cmd.value), begin(bulk), end(bulk)-1) ;
		check_command_output(cmd, "*3\r\n$3\r\nSET\r\n$11\r\nthis_is_key\r\n$13\r\nthis_is_value\r\n");
	}
	
	{
		redis::LPUSH<std::vector<std::string>> cmd;
		cmd.key = "this_is_key";
		cmd.values.push_back("this_is_value1");
		cmd.values.push_back("this_is_value2");
		check_command_output(cmd, "*4\r\n$5\r\nLPUSH\r\n$11\r\nthis_is_key\r\n$14\r\nthis_is_value1\r\n$14\r\nthis_is_value2\r\n");
	}
}

void test_key_value_command()
{
	redis::EXPIRE cmd;

	cmd.key = "this_is_key";
	cmd.time_to_live = 1000;

	check_command_output(cmd, "*3\r\n$6\r\nEXPIRE\r\n$11\r\nthis_is_key\r\n$4\r\n1000\r\n");
}

void test_adhoc_command()
{
	std::string key = "test";
	std::vector<int> test;
	test.push_back(1);
	test.push_back(2);
	auto cmd = redis::make_key_command(key, [&](redis::stream& output) {
		return redis::format_command(output, "SADD", key, redis::optional(true, test));
	});

	check_command_output(cmd, "*4\r\n$4\r\nSADD\r\n$4\r\ntest\r\n$1\r\n1\r\n$1\r\n2\r\n");
}

void test_string_command()
{
	// SETEX
	{
		redis::SETEX<int> cmd;
		cmd.key = "key";
		cmd.value = 100;
		cmd.time_to_live = 1000;

		check_command_output(cmd, "*4\r\n$5\r\nSETEX\r\n$3\r\nkey\r\n$4\r\n1000\r\n$3\r\n100\r\n");
	}

	// PSETEX
	{
		redis::PSETEX<const char*> cmd;
		cmd.key = "key";
		const char data[] = "data";
		cmd.value = data;
		cmd.time_to_live_ms = 10000;

		check_command_output(cmd, "*4\r\n$6\r\nPSETEX\r\n$3\r\nkey\r\n$5\r\n10000\r\n$4\r\ndata\r\n");
	}

	// GETRANGE
	{
		redis::GETRANGE cmd;
		cmd.key = "key";
		cmd.start = 0;
		cmd.end = -1;

		check_command_output(cmd, "*4\r\n$8\r\nGETRANGE\r\n$3\r\nkey\r\n$1\r\n0\r\n$2\r\n-1\r\n");
	}

	// SETRANGE
	{
		redis::SETRANGE<redis::const_buffer_view> cmd;
		cmd.key = "key";
		cmd.offset = 100;
		const char data[] = "data";
		cmd.value = redis::const_buffer_view(begin(data), end(data)-1);

		check_command_output(cmd, "*4\r\n$8\r\nSETRANGE\r\n$3\r\nkey\r\n$3\r\n100\r\n$4\r\ndata\r\n");
	}
}

void test_hash_command()
{
	// HSET
	{
		redis::HSET<int, std::string> cmd;
		cmd.key = "hello";
		cmd.field = 500;
		cmd.value = "value";

		check_command_output(cmd, "*4\r\n$4\r\nHSET\r\n$5\r\nhello\r\n$3\r\n500\r\n$5\r\nvalue\r\n");
	}

	// HSETNX
	{
		redis::HSETNX<std::vector<char>, int64_t> cmd;
		cmd.key = "hello";
		const char data[] = "field";
		cmd.field = std::vector<char>(begin(data), end(data)-1);
		cmd.value = INT64_MIN;
		check_command_output(cmd, "*4\r\n$6\r\nHSETNX\r\n$5\r\nhello\r\n$5\r\nfield\r\n$20\r\n-9223372036854775808\r\n");
	}

	// HMSET
	{
		redis::HMSET<uint64_t, std::vector<char>> cmd;
		cmd.key = "hello";
		const char data0[] = "value1";
		const char data1[] = "value2";
		cmd.key_value_list.push_back(std::make_pair(UINT64_MAX, std::vector<char>(begin(data0), end(data0)-1)));
		cmd.key_value_list.push_back(std::make_pair(0, std::vector<char>(begin(data1), end(data1)-1)));

		check_command_output(cmd, "*6\r\n$5\r\nHMSET\r\n$5\r\nhello\r\n$20\r\n18446744073709551615\r\n$6\r\nvalue1\r\n$1\r\n0\r\n$6\r\nvalue2\r\n");
	}
}

void test_list_command()
{
	// LINSERT
	{
		redis::LINSERT<std::string> cmd;
		cmd.key = "linsert_key";
		cmd.before_pivot = true;
		cmd.pivot = "pivot";
		cmd.value = "inserted";

		check_command_output(cmd, "*5\r\n$7\r\nLINSERT\r\n$11\r\nlinsert_key\r\n$6\r\nBEFORE\r\n$5\r\npivot\r\n$8\r\ninserted\r\n");

		cmd.before_pivot = false;

		check_command_output(cmd, "*5\r\n$7\r\nLINSERT\r\n$11\r\nlinsert_key\r\n$5\r\nAFTER\r\n$5\r\npivot\r\n$8\r\ninserted\r\n");
	}

	// LRANGE
	{
		redis::LRANGE cmd;
		cmd.key = "lrange_key";
		cmd.start = -3;
		cmd.stop = 2;

		check_command_output(cmd, "*4\r\n$6\r\nLRANGE\r\n$10\r\nlrange_key\r\n$2\r\n-3\r\n$1\r\n2\r\n");
	}

	// LTRIM
	{
		redis::LTRIM cmd;
		cmd.key = "ltrim_key";
		cmd.start = 2;
		cmd.stop = -10;

		check_command_output(cmd, "*4\r\n$5\r\nLTRIM\r\n$9\r\nltrim_key\r\n$1\r\n2\r\n$3\r\n-10\r\n");
	}

	// LREM
	{
		redis::LREM<std::string> cmd;
		cmd.key = "lrem_key";
		cmd.count = 100;
		cmd.value = "removed";

		check_command_output(cmd, "*4\r\n$4\r\nLREM\r\n$8\r\nlrem_key\r\n$3\r\n100\r\n$7\r\nremoved\r\n");
	}

	// LSET
	{
		redis::LSET<int> cmd;
		cmd.key = "lset_key";
		cmd.index = 100;
		cmd.value = 400;

		check_command_output(cmd, "*4\r\n$4\r\nLSET\r\n$8\r\nlset_key\r\n$3\r\n100\r\n$3\r\n400\r\n");
	}
}

void test_sorted_set_command()
{
	// ZADD
	{
		redis::ZADD<int, int> cmd;

		cmd.key = "test";
		cmd.score_member_list.push_back(std::make_pair(0, 0));
		cmd.score_member_list.push_back(std::make_pair(1, 1));
		cmd.score_member_list.push_back(std::make_pair(2, 2));

		check_command_output(cmd, "*8\r\n$4\r\nZADD\r\n$4\r\ntest\r\n$1\r\n0\r\n$1\r\n0\r\n$1\r\n1\r\n$1\r\n1\r\n$1\r\n2\r\n$1\r\n2\r\n");
	}

	// ZCOUNT
	{
		redis::ZCOUNT cmd;

		cmd.key = "test";
		cmd.min.trait = redis::interval_value::negative_inf;
		cmd.max.trait = redis::interval_value::inclusive;
		cmd.max.value = 100;

		check_command_output(cmd, "*4\r\n$6\r\nZCOUNT\r\n$4\r\ntest\r\n$4\r\n-inf\r\n$3\r\n100\r\n");
	}

	// ZRANGE
	{
		redis::ZRANGE cmd;

		cmd.key = "test";
		cmd.start = 10;
		cmd.stop = 100;

		check_command_output(cmd, "*4\r\n$6\r\nZRANGE\r\n$4\r\ntest\r\n$2\r\n10\r\n$3\r\n100\r\n");
	}

	// ZRANGEBYSCORE
	{
		redis::ZRANGEBYSCORE cmd1;

		cmd1.key = "test_key";
		cmd1.min.trait = redis::interval_value::negative_inf;
		cmd1.max.trait = redis::interval_value::positive_inf;
		cmd1.with_scores = true;
		cmd1.use_limit = true;
		cmd1.limit_count = 10;
		cmd1.limit_offset = 10;

		check_command_output(cmd1, "*8\r\n$13\r\nZRANGEBYSCORE\r\n$8\r\ntest_key\r\n$4\r\n-inf\r\n$4\r\n+inf\r\n$10\r\nWITHSCORES\r\n$5\r\nLIMIT\r\n$2\r\n10\r\n$2\r\n10\r\n");

		redis::ZRANGEBYSCORE cmd2;

		cmd2.key = "test_key";
		cmd2.min.trait = redis::interval_value::exclusive;
		cmd2.min.value = -100;
		cmd2.max.trait = redis::interval_value::inclusive;
		cmd2.max.value = 100;
		cmd2.with_scores = false;
		cmd2.use_limit = true;
		cmd2.limit_count = 10;
		cmd2.limit_offset = 10;

		check_command_output(cmd2, "*7\r\n$13\r\nZRANGEBYSCORE\r\n$8\r\ntest_key\r\n$5\r\n(-100\r\n$3\r\n100\r\n$5\r\nLIMIT\r\n$2\r\n10\r\n$2\r\n10\r\n");
	}

	// ZREMRANGEBYRANK
	{
		redis::ZREMRANGEBYRANK cmd;

		cmd.key = "test";
		cmd.start = 50;
		cmd.stop = 100;

		check_command_output(cmd, "*4\r\n$15\r\nZREMRANGEBYRANK\r\n$4\r\ntest\r\n$2\r\n50\r\n$3\r\n100\r\n");
	}

	// ZREMRANGEBYSCORE
	{
		redis::ZREMRANGEBYSCORE cmd;

		cmd.key = "test_key";
		cmd.min.trait = redis::interval_value::negative_inf;
		cmd.max.trait = redis::interval_value::positive_inf;

		check_command_output(cmd, "*4\r\n$16\r\nZREMRANGEBYSCORE\r\n$8\r\ntest_key\r\n$4\r\n-inf\r\n$4\r\n+inf\r\n");
	}

	// ZREVRANGE
	{
		redis::ZREVRANGE cmd;

		cmd.key = "test";
		cmd.start = 10;
		cmd.stop = 100;

		check_command_output(cmd, "*4\r\n$9\r\nZREVRANGE\r\n$4\r\ntest\r\n$2\r\n10\r\n$3\r\n100\r\n");
	}

	// ZREVRANGEBYSCORE
	{
		redis::ZREVRANGEBYSCORE cmd;

		cmd.key = "test_key";
		cmd.min.trait = redis::interval_value::inclusive;
		cmd.min.value = 100;
		cmd.max.trait = redis::interval_value::positive_inf;
		cmd.use_limit = true;
		cmd.limit_offset = 0;
		cmd.limit_count = 10;

		check_command_output(cmd, "*7\r\n$16\r\nZREVRANGEBYSCORE\r\n$8\r\ntest_key\r\n$3\r\n100\r\n$4\r\n+inf\r\n$5\r\nLIMIT\r\n$1\r\n0\r\n$2\r\n10\r\n");
	}
}

void test_pubsub_command()
{
	// TODO
}

void test_command()
{
	test_key_command();
	test_generic_key_value_command();
	test_key_value_command();
	test_adhoc_command();
	test_string_command();
	test_hash_command();
	test_list_command();
	test_sorted_set_command();
	test_pubsub_command();
}

} // the end of namespace "redis_test"
