#ifndef REDIS_TEST_H
#define REDIS_TEST_H

#include <vector>
#include <memory>
#include <algorithm>
#include <cstdint>
#include <cassert>

#include "redis_base.h"

namespace redis_test
{

struct reply
{
    enum type
    {
        null,
        status,
        error,
        integer_type,
        bulk_type,
        multi_bulk_type,
        ill_formed,
        reply_type_count,
    };

    reply() : t(reply_type_count) {}
    reply(reply&& r) : t(r.t) {
        switch (r.t) {
        case reply::ill_formed:
            return;
        case reply::status:
        case reply::error:
            str = std::move(r.str);
        case reply::integer_type:
            num = r.num;
        case reply::bulk_type:
            bulk = std::move(r.bulk);
        case reply::multi_bulk_type:
            multi_bulk = std::move(r.multi_bulk);
        }
    }
    
    type t;
    std::string str;
    int64_t num;
    std::vector<char> bulk;
    std::vector<std::unique_ptr<reply>> multi_bulk;
};



struct reply_builder : public redis::reply_handler
{
    reply_builder() : depth(-1)
    {
    }
    virtual ~reply_builder() {}

    virtual bool on_status(redis::const_buffer_view data) override
    {
        std::unique_ptr<reply> r(new reply);
        r->t = reply::status;
        r->str = std::string(begin(data), end(data));
        push(std::move(r));
        return true;
    }

    virtual bool on_error(redis::const_buffer_view data) override
    {
        std::unique_ptr<reply> r(new reply);
        r->t = reply::error;
        r->str = std::string(begin(data), end(data));
        push(std::move(r));
        return true;
    }

    virtual bool on_integer(int64_t value) override
    {
        std::unique_ptr<reply> r(new reply);
        r->t = reply::integer_type;
        r->num = value;
        push(std::move(r));
        return true;
    }

    virtual bool on_null() override
    {
        std::unique_ptr<reply> r;
        push(std::move(r));
        return true;
    }

    virtual bool on_bulk(redis::const_buffer_view data) override
    {
        std::unique_ptr<reply> r(new reply);
        r->t = reply::bulk_type;
        r->bulk = std::vector<char>(begin(data), end(data));
        push(std::move(r));
        return true;
    }

    virtual bool on_multi_bulk_begin(size_t count) override
    {
        std::unique_ptr<reply> r(new reply);
        r->t = reply::multi_bulk_type;
        r->multi_bulk.reserve(count);
        push(std::move(r));
        return true;
    }

    virtual bool on_enter_reply(size_t recursion_depth) override
    {
        depth = recursion_depth;
        stack.resize(depth+1);
        return true;
    }

    virtual bool on_leave_reply(size_t recursion_depth) override
    {
        depth = recursion_depth-1;
        stack.resize(depth+1);
        return true;
    }

    void push(std::unique_ptr<reply> r)
    {
        stack.back() = r.get();
        if (depth > 0) {
            stack[depth-1]->multi_bulk.push_back(std::move(r));
        } else {
            root = std::move(r);
        } 
    }

    std::vector<reply*> stack;
    std::unique_ptr<reply> root;
    size_t depth;
};


struct mock_stream : public redis::stream
{
    mock_stream() : input_offset(0), is_opened(true)
    {
    }

    virtual ~mock_stream()
    {
    }

    // implementation of redis::stream interface
    virtual size_t available() const override
    {
        return input_buffer.size() - input_offset;
    }

    virtual redis::const_buffer_view peek(size_t n) override
    {
        if (input_offset >= input_buffer.size()) {
            return redis::const_buffer_view();
        }
        return redis::const_buffer_view(current(), possible_end(n));
    }

    virtual redis::const_buffer_view read(size_t n) override
    {
        if (input_offset >= input_buffer.size()) {
            return redis::const_buffer_view();
        }
        auto begin = current();
        auto end = possible_end(n);
        skip(n);
        return redis::const_buffer_view(begin, end);
    }

    virtual size_t skip(size_t n) override
    {
        auto delta = std::min(input_buffer.size()-input_offset, n);
        assert(delta <= input_buffer.size()-input_offset);
        input_offset += delta;
        return delta;
    }

    virtual bool is_open() const override
    {
        return is_opened;
    }

    virtual bool close() override
    {
        is_opened = false;
        return true;
    }

    virtual bool flush() override
    {
        flushed_offsets.push_back(output_buffer.size());
        return true;
    }

    virtual bool write(redis::const_buffer_view input) override
    {
        output_buffer.insert(output_buffer.end(), input.begin(), input.end());
        return true;
    }


    // helper functions
    void more_input(const char input[])
    {
        input_buffer.insert(end(input_buffer), input, input+strlen(input));
    }

    void more_input(const std::vector<char>& input)
    {
        input_buffer.insert(end(input_buffer), begin(input), end(input));
    }

    char* current()
    {
        assert(input_offset < input_buffer.size());
        return input_buffer.data() + input_offset;
    }

    char* possible_end(size_t n)
    {
        return current() + std::min(n, available());
    }

    size_t input_offset;
    std::vector<char> input_buffer;
    bool is_opened;

    std::vector<size_t> flushed_offsets;
    std::vector<char> output_buffer;
};


bool operator== (const reply& lhs, const reply& rhs);
inline bool operator!= (const reply& lhs, const reply& rhs)
{
    return !(lhs == rhs);
}

void serialize(const reply& r, redis::stream& output);
const bool check_equal(const char expected[], mock_stream& output);


} // namespace "redis_test"

#endif // REDIS_TEST_H
