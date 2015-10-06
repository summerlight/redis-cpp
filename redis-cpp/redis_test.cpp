#include "redis_test.h"

#include <string>
#include <iterator>
#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <cinttypes>

namespace redis_test
{

using std::begin;
using std::end;

const bool check_equal(const char expected[], mock_stream& output)
{
	auto expected_size = strlen(expected);
	return std::equal(begin(output.output_buffer), end(output.output_buffer),
		expected, expected + expected_size);
}

void serialize(const reply& r, redis::stream& output)
{
	char buffer[256];
	switch (r.t) {
	case reply::null:
		snprintf(buffer, sizeof(buffer), "$-1\r\n");
		break;
	case reply::status:
		snprintf(buffer, sizeof(buffer), "+%s\r\n", r.str.c_str());
		break;
	case reply::error:
		snprintf(buffer, sizeof(buffer), "-%s\r\n", r.str.c_str());
		break;
	case reply::integer_type:
		snprintf(buffer, sizeof(buffer), ":%" PRId64 "\r\n", r.num);
		break;
	case reply::bulk_type:
		snprintf(buffer, sizeof(buffer), "-%d", r.bulk.size());
		// \r\n
		break;
	case reply::multi_bulk_type:
		snprintf(buffer, sizeof(buffer), "*%d", r.multi_bulk.size());
		// \r\n
		for (auto i = begin(r.multi_bulk), e = end(r.multi_bulk); i != e; ++i) {
			serialize(*i->get(), output);
		}
	}
}

bool operator== (const reply& lhs, const reply& rhs)
{
	if (lhs.t != rhs.t) {
		return false;
	}
	switch (lhs.t) {
	case reply::ill_formed:
		return false;
	case reply::status:
	case reply::error:
		return lhs.str == rhs.str;
	case reply::integer_type:
		return lhs.num == rhs.num;
	case reply::bulk_type:
		return lhs.bulk == rhs.bulk;
	case reply::multi_bulk_type:
		return lhs.multi_bulk.size() == rhs.multi_bulk.size() &&
			std::equal(begin(lhs.multi_bulk), end(lhs.multi_bulk), begin(rhs.multi_bulk),
			[](const std::unique_ptr<reply>& lhs, const std::unique_ptr<reply>& rhs) -> bool {
				if (lhs == nullptr || rhs == nullptr) {
					return lhs == rhs;
				} else {
					return *lhs == *rhs;
				}
			});
	}
	assert(false);
	return false;
}

} // the end of namespace "redis_test"
