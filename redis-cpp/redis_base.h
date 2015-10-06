#ifndef REDIS_BASE_H
#define REDIS_BASE_H

#include <cstdint>
#include <type_traits>
#include <system_error>

#include "array_view.h"
#include "error.h"

namespace redis
{

const char crlf[] = {'\r', '\n'};

typedef array_view<char> buffer_view;
typedef array_view<const char> const_buffer_view;


struct stream
{
    virtual ~stream() {}

    virtual bool close() = 0;
    virtual bool is_open() const = 0;

    // interface for stream input
    virtual size_t available() const = 0;

    virtual const_buffer_view peek(size_t n) = 0;
    virtual const_buffer_view read(size_t n) = 0;
    virtual size_t skip(size_t n) = 0;

    // interface for stream output
    virtual bool flush() = 0;

    virtual bool write(const_buffer_view input) = 0;

    // utility member functions
    template<typename T>
    bool read(T& value) {
        static_assert(std::is_pod<T>::value, "Only pod type can be directly read from stream.");
        auto result = read(sizeof(T));
        if (result.valid() && result.size() == sizeof(T)) {
            std::copy(begin(result), end(result), static_cast<char*>(static_cast<void*>(&value)));
            return true;
        } else {
            return false;
        }
    }

    template<typename T>
    bool write(const T& value) {
        static_assert(std::is_pod<T>::value, "Only pod type can be directly written to stream.");
        return write(const_buffer_view(&value, sizeof(T)));
    }
};


struct command
{
    virtual std::error_code write_command(stream& output) const = 0;

    // to prepare cluster use
    virtual const_buffer_view cluster_key() const = 0;
    virtual bool is_subscriber_cmd() const = 0;
};


struct reply_handler
{
public:
    virtual ~reply_handler() {}

    // boolean return value of each handler function means whether input is expected or not
    // if the function returns 'false' then parser stops calling handler functions
    virtual bool on_status(const_buffer_view data) = 0;
    virtual bool on_error(const_buffer_view data) = 0;
    virtual bool on_integer(int64_t value) = 0;
    virtual bool on_null() = 0;
    virtual bool on_bulk(const_buffer_view data) = 0;
    virtual bool on_multi_bulk_begin(size_t count) = 0;

    virtual bool on_enter_reply(size_t recursion_depth) = 0;
    virtual bool on_leave_reply(size_t recursion_depth) = 0;
};

// reply parse function
std::error_code parse(stream& input, reply_handler& handler);


// thread-safety : safe in distinct, not safe in shared
template<typename stream_type>
struct session : public stream_type
{
    session() {}
    ~session() {}

    template<typename command_type>
    std::error_code request(command_type& cmd)
    {
        return request(cmd, cmd.reply);
    }

    std::error_code request(const command& cmd, reply_handler& handler)
    {
        if (!is_open()) {
            return redis::error::stream_not_initialized;
        }

        if (cmd.is_subscriber_cmd()) {
            return redis::error::subscriber_cmd_error;
        }

        auto ec = cmd.write_command(*this);
        if (ec) {
            return close() ? redis::error::stream_error : ec;
        }

        if (!flush()) {
            close();
            return redis::error::stream_error;
        }

        ec = redis::parse(*this, handler);
        if (ec) {
            return close() ? redis::error::stream_error : ec;
        }

        return std::error_code();
    }
};

} // namespace "redis"

#endif // REDIS_BASE_H
