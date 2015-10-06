#include <cassert>
#include <vector>
#include <algorithm>

#include "redis_base.h"
#include "finally.h"
#include "error.h"

namespace redis {

using std::begin;
using std::end;

// synchronous parser routines
// currently, asynchronous input stream is not considered - every stream implementation should return full data expected in the request

namespace {

class parser
{
public:
    parser(stream& input, reply_handler& handler)
        : stream(input), handler(handler), recursion_depth(0), handler_error(false), reply_error(false)
    {
    }
            
    template<typename func>
    void handle(func f)
    {
        if (!handler_error && !(handler.*f)()) {
            handler_error = true;
            err = error::handler_error;
        }
    }

    template<typename func, typename T1>
    void handle(func f, T1&& v1)
    {
        if (!handler_error && !(handler.*f)(std::forward<T1>(v1))) {
            handler_error = true;
            err = error::handler_error;
        }
    }

    template<typename func, typename T1, typename T2>
    void handle(func&& f, T1&& v1, T2&& v2)
    {
        if (!handler_error && !(handler.*f)(std::forward<T1>(v1), std::forward<T2>(v2))) {
            handler_error = true;
            err = error::handler_error;
        }
    }

    template<typename char_iterator>
    bool read_integer(char_iterator i, char_iterator e, int64_t& output)
    {
        int32_t value = 0;
        auto signedness = 1;

        if (*i == '-') {
            signedness = -1;
            ++i;
        } else if (*i == '+') {
            ++i;
        }

        for (; i != e; ++i) {
            auto decimal = *i - '0';
            if (decimal >= 0 && decimal < 10) {
                value *= 10;
                value += decimal;
            } else {
                err = error::ill_formed_reply;
                return false;
            }
        }

        output = signedness * value;
        return true;
    }

    bool read_crlf()
    {
        // Don't need to check crlf, just skip 2 byte
        auto result = stream.skip(sizeof(crlf));
        if (result != sizeof(crlf)) {
            err = error::stream_error;
            return false;
        }
        return true;
    }

    template<typename functor>
    bool read_line(functor func)
    {
        size_t msg_size = 64; // There's no message reply over 64 byte in redis currently...

        while(true) { // Though we should handle arbitrary sized message
            auto buffer = stream.peek(msg_size);
            if (!buffer.valid()) {
                err = error::stream_error;
                return false;
            }
            auto search_result = std::search(begin(buffer), end(buffer), begin(crlf), end(crlf));

            if (search_result != end(buffer)) {
                auto line_data = buffer.slice(0, search_result - begin(buffer));
                if (!func(line_data)) {
                    return false;
                }
                stream.skip(search_result - begin(buffer));		
                return read_crlf();
            } else if (buffer.size() == msg_size) { // If 64 byte is not enough
                msg_size *= 2; // Try once more with doubled buffer
            } else {
                err = error::stream_error;
                return false;
            }
        }		
    }

    bool read_bulk()
    {
        int64_t expected_size = 0;
        auto result = read_line([&](const const_buffer_view& buffer) {
            return read_integer(begin(buffer), end(buffer), expected_size);
        });

        if (!result) {
            return false;
        }

        if (expected_size < 0) {
            handle(&reply_handler::on_null);
            return true;
        } else {
            auto buffer = stream.read(static_cast<size_t>(expected_size));
            if (!buffer.valid() || buffer.size() != expected_size) {
                err = error::stream_error;
                return false;
            }
            handle(&reply_handler::on_bulk, buffer);
            return read_crlf();
        }	
    }

    bool read_multi_bulk()
    {
        int64_t expected_bulk_count = 0;
        auto result = read_line([&](const const_buffer_view& buffer) {
            return read_integer(begin(buffer), end(buffer), expected_bulk_count);
        });

        if (!result) {
            return false;
        }

        handle(&reply_handler::on_multi_bulk_begin, static_cast<size_t>(expected_bulk_count));

        for (int i = 0; i < expected_bulk_count; i++) {
            if (!parse_one_reply()) {
                return false;
            }
        }
        return true;
    }

    bool parse_one_reply()
    {
        char type;
        if (!stream.read(type)) {
            err = error::stream_error;
            return false;
        }

        handle(&reply_handler::on_enter_reply, recursion_depth++);
        auto on_leave = finally([this] {
            handle(&reply_handler::on_leave_reply, --recursion_depth);
        });

        switch(type)
        {
        case '+': // Single line reply
            return read_line([this](const const_buffer_view& buffer) -> bool {
                handle(&reply_handler::on_status, buffer);
                return true;
            });
        case '-': // Error message
            reply_error = true;
            err = error::error_reply;
            return read_line([this](const const_buffer_view& buffer) -> bool {
                handle(&reply_handler::on_error, buffer);
                return true;
            });
        case ':': // Integer number
            return read_line([this](const const_buffer_view& buffer) -> bool {
                int64_t value = 0;
                if (!read_integer(begin(buffer), end(buffer), value)) {
                    return false;
                }
                handle(&reply_handler::on_integer, value);
                return true;
            });
        case '$': // Bulk reply
            return read_bulk();
        case '*': // Multi-bulk reply
            return read_multi_bulk();
        default : // Ill-formed reply
            err = error::ill_formed_reply;
            return false;
        }
    }

    bool parse()
    {
        return parse_one_reply() && !handler_error && !reply_error;
    }

    stream& stream;
    reply_handler& handler;
    size_t recursion_depth;
    std::error_code err;
    bool handler_error;
    bool reply_error;
};

} // the end of anonymous namespace

std::error_code parse(stream& input, reply_handler& handler)
{
    parser p(input, handler);

    if (p.parse()) {
        return std::error_code();
    } else {
        return p.err;
    }
}

} // namespace "redis"
