#ifndef REDIS_REPLY_H
#define REDIS_REPLY_H

#include "redis_base.h"

namespace redis
{

// base class which defines default behavior for reply handler
struct reply_handler_base : public reply_handler
{
public:
	reply_handler_base()
	{
	}

	// implementation of reply_handler interface
	virtual bool on_status(const_buffer_view data) override
	{
		return false;
	}

	virtual bool on_error(const_buffer_view data) override
	{
		error_info = std::string(begin(data), end(data));
		return true;
	}

	virtual bool on_integer(int64_t value) override
	{
		return false;
	}

	virtual bool on_bulk(const_buffer_view data) override
	{
		return false;
	}

	virtual bool on_null() override
	{
		return false;
	}

	virtual bool on_multi_bulk_begin(size_t count) override
	{
		return false;
	}
	
	virtual bool on_enter_reply(size_t recursion_depth) override
	{
		if (recursion_depth > 1) {
			return false; // default commands does not generate recursive reply
		}
		return true;
	}
	virtual bool on_leave_reply(size_t recursion_depth) override
	{
		if (recursion_depth > 1) {
			return false; // default commands does not generate recursive reply
		}
		return true;
	}

	std::string error_info;
};


struct bulk_data
{
	bulk_data() : is_null(true) {}
	bulk_data(const char* begin, const char* end) : data(begin, end) {}

	bool is_null;
	std::vector<char> data;
};

// default handlers
struct status_reply : public reply_handler_base
{
public:
	virtual bool on_status(const_buffer_view data) override
	{
		status = std::string(data.begin(), data.end());
		return true;
	}

	std::string status;
};

struct boolean_reply : public reply_handler_base
{
public:
	boolean_reply() : result(false) {}

	virtual bool on_status(const_buffer_view data) override
	{
		result = true;
		return true;
	}

	virtual bool on_integer(int64_t value) override
	{
		result = (value != 0);
		return true;
	}

	bool result;
};

struct integer_reply : public reply_handler_base
{
public:
	integer_reply() : result(-1) {}

	virtual bool on_integer(int64_t value) override
	{
		result = value;
		return true;
	}

	int64_t result;
};

struct bulk_reply : public reply_handler_base
{
public:
	bulk_reply() {}

	virtual bool on_bulk(const_buffer_view data) override
	{
		result.is_null = false;
		result.data = std::vector<char>(data.begin(), data.end());
		return true;
	}

	virtual bool on_null() override
	{
		return true;
	}

	bulk_data result;
};

struct multi_bulk_reply : public reply_handler_base
{
public:
	multi_bulk_reply() {}

	virtual bool on_multi_bulk_begin(size_t count) override
	{
		result.reserve(count);
		return true;
	}

	virtual bool on_bulk(const_buffer_view data) override
	{
		result.push_back(bulk_data(data.begin(), data.end()));
		return true;
	}

	virtual bool on_null() override
	{
		result.push_back(bulk_data());
		return true;
	}

	std::vector<bulk_data> result;
};

struct rank_reply : public reply_handler_base
{
public:
	rank_reply() : is_null(false), result(-1) {}

	virtual bool on_integer(int64_t value) override
	{
		result = value;
		return true;
	}

	virtual bool on_null() override
	{
		return true;
	}

	bool is_null;
	int64_t result;
};

} // the end of namespace "redis"

#endif // REDIS_REPLY_H
