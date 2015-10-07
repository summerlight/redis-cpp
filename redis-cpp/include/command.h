#ifndef REDIS_COMMAND_H
#define REDIS_COMMAND_H

#include <iterator>
#include <string>
#include <vector>

#include <cstddef>
#include <cstdint>

#include "redis_base.h"
#include "error.h"
#include "writer_type_traits.h"
#include "reply.h"

namespace redis
{

struct single_key_command : public command
{
    virtual const_buffer_view cluster_key() const override
    {
        return const_buffer_view(key.data(), key.size());
    }

    virtual bool is_subscriber_cmd() const
    {
        return false;
    }

    std::string key;
};


struct subscriber_command : public command
{
    virtual const_buffer_view cluster_key() const override
    {
        return const_buffer_view();
    }

    virtual bool is_subscriber_cmd() const
    {
        return true;
    }
};

// ad-hoc command
template<typename cmd_functor>
struct adhoc_key_command : public command
{
    adhoc_key_command(const std::string& key, cmd_functor&& cmd) : key(key), cmd(cmd)
    {
    }

    virtual std::error_code write_command(stream& output) const override
    {
        return cmd(output);
    }

    virtual const_buffer_view cluster_key() const override
    {
        return const_buffer_view(key.data(), key.size());
    }

    virtual bool is_subscriber_cmd() const
    {
        return false;
    }

    const std::string& key;
    cmd_functor cmd;
};

template<typename cmd_functor>
inline adhoc_key_command<cmd_functor> make_key_command(const std::string& key, cmd_functor&& cmd)
{
    return adhoc_key_command<cmd_functor>(key, std::forward<cmd_functor>(cmd));
}


template<typename cmd_functor>
struct adhoc_command : public command
{
    adhoc_command(cmd_functor&& cmd) : key(key), cmd(cmd)
    {
    }

    virtual std::error_code write_command(stream& output) const override
    {
        return cmd(output);
    }

    virtual const_buffer_view cluster_key() const override
    {
        return const_buffer_view();
    }

    virtual bool is_subscriber_cmd() const
    {
        return false;
    }

    cmd_functor cmd;
};

template<typename cmd_functor>
inline adhoc_command<cmd_functor> make_command(cmd_functor&& cmd)
{
    return adhoc_command<cmd_functor>(std::forward<cmd_functor>(cmd));
}


#define DECLARE_KEY_CMD(cmd_name, handler_type)\
struct cmd_name : public single_key_command\
{\
    virtual std::error_code write_command(stream& output) const override\
    {\
        return format_command(output, #cmd_name, key);\
    }\
    \
    handler_type reply;\
};

#define DECLARE_KEY_VALUE_CMD(cmd_name, value_type, value_name, handler_type)\
struct cmd_name : public single_key_command\
{\
    cmd_name() : value_name(value_type()) {}\
    virtual std::error_code write_command(stream& output) const override\
    {\
        return format_command(output, #cmd_name, key, value_name);\
    }\
    \
    value_type value_name;\
    handler_type reply;\
}

#define DECLARE_GENERIC_KEY_VALUE_CMD(cmd_name, value_name, handler_type)\
template<typename T>\
struct cmd_name : public single_key_command\
{\
    cmd_name() : value_name(T()) {}\
    virtual std::error_code write_command(stream& output) const override\
    {\
        if (count_element(value_name) == 0) {\
            return error::invalid_command_format;\
        }\
        return format_command(output, #cmd_name, key, value_name);\
    }\
\
    T value_name;\
    handler_type reply;\
}

#define DECLARE_GENERIC_KEY_SINGLE_VALUE_CMD(cmd_name, value_name, handler_type)\
template<typename T>\
struct cmd_name : public single_key_command\
{\
    static_assert(is_single_element_type<T>::value, "T should be represented in a single string.");\
    cmd_name() : value_name(T()) {}\
    virtual std::error_code write_command(stream& output) const override\
    {\
        return format_command(output, #cmd_name, key, value_name);\
    }\
\
    T value_name;\
    handler_type reply;\
}


// key-related commands
DECLARE_KEY_CMD(DEL, integer_reply); // make it single key command to support cluster
DECLARE_KEY_CMD(EXISTS, boolean_reply);
DECLARE_KEY_CMD(PERSIST, boolean_reply);
DECLARE_KEY_CMD(TYPE, status_reply);
DECLARE_KEY_VALUE_CMD(EXPIRE, int32_t, time_to_live, boolean_reply);
DECLARE_KEY_VALUE_CMD(PEXPIRE, int32_t, time_to_live_ms, boolean_reply);
DECLARE_KEY_VALUE_CMD(EXPIREAT, int32_t, expire_time, boolean_reply);
DECLARE_KEY_VALUE_CMD(PEXPIREAT, int32_t, expire_time_ms, boolean_reply);
DECLARE_KEY_CMD(TTL, integer_reply);
DECLARE_KEY_CMD(PTTL, integer_reply);

// string-related commands
DECLARE_GENERIC_KEY_SINGLE_VALUE_CMD(APPEND, value, integer_reply);
DECLARE_KEY_CMD(GET, bulk_reply);
DECLARE_KEY_CMD(STRLEN, integer_reply);
DECLARE_GENERIC_KEY_SINGLE_VALUE_CMD(SET, value, boolean_reply);
DECLARE_GENERIC_KEY_SINGLE_VALUE_CMD(GETSET, value, bulk_reply);
DECLARE_GENERIC_KEY_SINGLE_VALUE_CMD(SETNX, value, boolean_reply);

template<typename T>
struct SETEX : public single_key_command
{
    static_assert(is_single_element_type<T>::value, "T should be represented in a single string.");
    
    SETEX() : time_to_live(0), value(T()) {}

    virtual std::error_code write_command(stream& output) const override
    {
        return format_command(output, "SETEX", key, time_to_live, value);
    }

    int32_t time_to_live;
    T value;
    boolean_reply reply;
};

template<typename T>
struct PSETEX : public single_key_command
{
    static_assert(is_single_element_type<T>::value, "T should be represented in a single string.");
    
    PSETEX() : time_to_live_ms(0), value(T()) {}

    virtual std::error_code write_command(stream& output) const override
    {
        return format_command(output, "PSETEX", key, time_to_live_ms, value);
    }

    int32_t time_to_live_ms;
    T value;
    boolean_reply reply;
};

struct GETRANGE : public single_key_command
{
    GETRANGE() : start(0), end(0) {}

    virtual std::error_code write_command(stream& output) const override
    {
        return format_command(output, "GETRANGE", key, start, end);
    }

    int32_t start;
    int32_t end;
    bulk_reply reply;
};

template<typename T>
struct SETRANGE : public single_key_command
{
    static_assert(is_single_element_type<T>::value, "T should be represented in a single string.");
    
    SETRANGE() : offset(0), value(T()) {}

    virtual std::error_code write_command(stream& output) const override
    {
        return format_command(output, "SETRANGE", key, offset, value);
    }

    int32_t offset;
    T value;
    integer_reply reply;
};


// hash-related commands
DECLARE_KEY_VALUE_CMD(HDEL, std::vector<std::string>, fields, integer_reply);
DECLARE_KEY_VALUE_CMD(HEXISTS, std::string, field, boolean_reply);
DECLARE_KEY_VALUE_CMD(HGET, std::string, field, bulk_reply);
DECLARE_KEY_CMD(HGETALL, multi_bulk_reply);
DECLARE_KEY_CMD(HKEYS, multi_bulk_reply);
DECLARE_KEY_CMD(HVALS, multi_bulk_reply);
DECLARE_KEY_CMD(HLEN, integer_reply);
DECLARE_KEY_VALUE_CMD(HMGET, std::vector<std::string>, fields, multi_bulk_reply);

template<typename key_type, typename value_type>
struct HSET : public single_key_command
{
    static_assert(is_single_element_type<key_type>::value, "key_type should be represented in a single string.");
    static_assert(is_single_element_type<value_type>::value, "value_type should be represented in a single string.");

    HSET() : field(key_type()), value(value_type()) {}

    virtual std::error_code write_command(stream& output) const override
    {
        return format_command(output, "HSET", key, field, value);
    }

    key_type field;
    value_type value;
    boolean_reply reply;
};

template<typename key_type, typename value_type>
struct HSETNX : public single_key_command
{
    static_assert(is_single_element_type<key_type>::value, "key_type should be represented in a single string.");
    static_assert(is_single_element_type<value_type>::value, "value_type should be represented in a single string.");

    HSETNX() : field(key_type()), value(value_type()) {}

    virtual std::error_code write_command(stream& output) const override
    {
        return format_command(output, "HSETNX", key, field, value);
    }

    key_type field;
    value_type value;
    boolean_reply reply;
};

template<typename key_type, typename value_type>
struct HMSET : public single_key_command
{
    static_assert(is_single_element_type<key_type>::value, "key_type should be represented in a single string.");
    static_assert(is_single_element_type<value_type>::value, "value_type should be represented in a single string.");

    virtual std::error_code write_command(stream& output) const override
    {
        if (key_value_list.empty()) {
            return error::invalid_command_format;
        }
        return format_command(output, "HMSET", key, key_value_list);
    }

    typedef std::pair<key_type, value_type> pair_type;
    std::vector<pair_type> key_value_list;
    boolean_reply reply;
};

// list-related commands
DECLARE_KEY_VALUE_CMD(LINDEX, int32_t, index, bulk_reply);
DECLARE_KEY_CMD(LLEN, integer_reply);

DECLARE_KEY_CMD(LPOP, bulk_reply);
DECLARE_GENERIC_KEY_VALUE_CMD(LPUSH, values, integer_reply);
DECLARE_GENERIC_KEY_SINGLE_VALUE_CMD(LPUSHX, values, integer_reply);

DECLARE_KEY_CMD(RPOP, bulk_reply);
DECLARE_GENERIC_KEY_VALUE_CMD(RPUSH, values, integer_reply);
DECLARE_GENERIC_KEY_SINGLE_VALUE_CMD(RPUSHX, values, integer_reply);

template<typename pivot_type, typename value_type = pivot_type> 
struct LINSERT : public single_key_command
{
    static_assert(is_single_element_type<pivot_type>::value, "pivot_type should be represented in a single string.");
    static_assert(is_single_element_type<value_type>::value, "value_type should be represented in a single string.");

    LINSERT() : before_pivot(false), pivot(pivot_type()), value(value_type()) {}

    virtual std::error_code write_command(stream& output) const override
    {
        return format_command(output, "LINSERT", key, before_pivot ? "BEFORE" : "AFTER", pivot, value);
    }

    bool before_pivot;
    pivot_type pivot;
    value_type value;
    integer_reply reply;
};

struct LRANGE : public single_key_command
{
    LRANGE() : start(0), stop(0) {}

    virtual std::error_code write_command(stream& output) const override
    {
        return format_command(output, "LRANGE", key, start, stop);
    }

    int32_t start;
    int32_t stop;
    multi_bulk_reply reply;
};

struct LTRIM : public single_key_command
{
    LTRIM() : start(0), stop(0) {}

    virtual std::error_code write_command(stream& output) const override
    {
        return format_command(output, "LTRIM", key, start, stop);
    }

    int32_t start;
    int32_t stop;
    boolean_reply reply;
};

template<typename T>
struct LREM : public single_key_command
{
    static_assert(is_single_element_type<T>::value, "T should be represented in a single string.");

    LREM() : count(0), value(T()) {}

    virtual std::error_code write_command(stream& output) const override
    {
        return format_command(output, "LREM", key, count, value);
    }

    // count > 0 : head -> tail
    // count < 0 : tail -> head
    // count == 0 : remove all elements which == value
    int32_t count; 
    T value;
    integer_reply reply;
};

template<typename T>
struct LSET : public single_key_command
{
    static_assert(is_single_element_type<T>::value, "T should be represented in a single string.");

    LSET() : index(0), value(T()) {}

    virtual std::error_code write_command(stream& output) const override
    {		
        return format_command(output, "LSET", key, index, value);
    }

    int32_t index; 
    T value;
    boolean_reply reply;
};

// set-related commands
DECLARE_GENERIC_KEY_VALUE_CMD(SADD, members, integer_reply);
DECLARE_KEY_CMD(SCARD, integer_reply);
DECLARE_GENERIC_KEY_SINGLE_VALUE_CMD(SISMEMBER, member, boolean_reply);
DECLARE_KEY_CMD(SMEMBERS, multi_bulk_reply);
DECLARE_GENERIC_KEY_VALUE_CMD(SREM, member, integer_reply);

// sorted set-related commands

struct interval_value
{
    interval_value() : value(0), trait(inclusive) {}

    enum interval_trait {
        inclusive,
        exclusive,
        negative_inf,
        positive_inf,
        trait_count
    };

    int32_t value;
    interval_trait trait;
};

template<>
struct writer_type_traits<interval_value>
{
    static const bool static_count = true;
    static const size_t count = 1;
    static bool write(stream& output, interval_value interval)
    {
        switch(interval.trait)
        {
        case interval_value::exclusive:
        {
            char buffer[24];
            buffer[0] = '(';
            auto result = detail::write_int_on_buf(buffer_view(begin(buffer)+1, end(buffer)), interval.value);
            return write_element(output, const_buffer_view(buffer, result.end()));
        }
        case interval_value::inclusive:
        {
            char buffer[24];
            return write_element(output, detail::write_int_on_buf(buffer_view(begin(buffer), end(buffer)), interval.value));
        }
        case interval_value::negative_inf:
        {
            const static char neg_inf[] = "-inf";
            return write_element(output, neg_inf);
        }
        case interval_value::positive_inf:
        {
            const static char pos_inf[] = "+inf";
            return write_element(output, pos_inf);
        }
        default:
            assert(false);
        }
        return false;
    }
};

DECLARE_KEY_CMD(ZCARD, integer_reply);
DECLARE_GENERIC_KEY_SINGLE_VALUE_CMD(ZRANK, member, rank_reply);
DECLARE_GENERIC_KEY_VALUE_CMD(ZREM, member, integer_reply);
DECLARE_GENERIC_KEY_SINGLE_VALUE_CMD(ZREVRANK, member, rank_reply);
DECLARE_GENERIC_KEY_SINGLE_VALUE_CMD(ZSCORE, member, bulk_reply);

template<typename score_type, typename member_type>
struct ZADD : public single_key_command
{
    static_assert(std::is_arithmetic<score_type>::value, "Score type of sorted set should be an arithmetic type.");

    virtual std::error_code write_command(stream& output) const override
    {
        if (score_member_list.empty()) {
            return error::invalid_command_format;
        }
        return format_command(output, "ZADD", key, score_member_list);
    }

    typedef std::pair<score_type, member_type> pair_type;
    std::vector<pair_type> score_member_list;
    integer_reply reply;
};

struct ZCOUNT : public single_key_command
{
    virtual std::error_code write_command(stream& output) const override
    {
        return format_command(output, "ZCOUNT", key, min, max);
    }

    interval_value min;
    interval_value max;
    integer_reply reply;
};

struct ZRANGE : public single_key_command
{
    ZRANGE() : start(0), stop(0), with_scores(false) {}

    virtual std::error_code write_command(stream& output) const override
    {
        return format_command(output, "ZRANGE", key, start, stop, optional(with_scores, "WITHSCORES"));
    }

    int32_t start;
    int32_t stop;
    bool with_scores;
    multi_bulk_reply reply;
};

struct ZRANGEBYSCORE : public single_key_command
{
    ZRANGEBYSCORE() : with_scores(false), use_limit(false), limit_offset(0), limit_count(0) {}

    virtual std::error_code write_command(stream& output) const override
    {
        return format_command(output, "ZRANGEBYSCORE", key, min, max,
            optional(with_scores, "WITHSCORES"),
            optional(use_limit, "LIMIT", limit_offset, limit_count));
    }

    interval_value min;
    interval_value max;
    bool with_scores;
    bool use_limit;
    int32_t limit_offset;
    int32_t limit_count;
    multi_bulk_reply reply;
};


struct ZREMRANGEBYRANK : public single_key_command
{
    ZREMRANGEBYRANK() : start(0), stop(0) {}

    virtual std::error_code write_command(stream& output) const override
    {
        return format_command(output, "ZREMRANGEBYRANK", key, start, stop);
    }

    int32_t start;
    int32_t stop;
    integer_reply reply;
};

struct ZREMRANGEBYSCORE : public single_key_command
{
    virtual std::error_code write_command(stream& output) const override
    {
        return format_command(output, "ZREMRANGEBYSCORE", key, min, max);
    }

    interval_value min;
    interval_value max;
    integer_reply reply;
};

struct ZREVRANGE : public single_key_command
{
    ZREVRANGE() : start(0), stop(0), with_scores(false) {}

    virtual std::error_code write_command(stream& output) const override
    {
        return format_command(output, "ZREVRANGE", key, start, stop, optional(with_scores, "WITHSCORES"));
    }

    int32_t start;
    int32_t stop;
    bool with_scores;
    multi_bulk_reply reply;
};

struct ZREVRANGEBYSCORE : public single_key_command
{
    ZREVRANGEBYSCORE() : with_scores(false), use_limit(false), limit_offset(0), limit_count(0) {}

    virtual std::error_code write_command(stream& output) const override
    {
        return format_command(output, "ZREVRANGEBYSCORE", key, min, max,
            optional(with_scores, "WITHSCORES"),
            optional(use_limit, "LIMIT", limit_offset, limit_count));
    }

    interval_value min;
    interval_value max;
    bool with_scores;
    bool use_limit;
    int32_t limit_offset;
    int32_t limit_count;
    multi_bulk_reply reply;
};


// pub/sub-related commands
// TODO : how to deal with cluster?
template<typename T>
struct PSUBSCRIBE : public subscriber_command
{
    PSUBSCRIBE() : pattern(T()) {}

    virtual std::error_code write_command(stream& output) const override
    {
        if (count_element(pattern) == 0) {
            return error::invalid_command_format;
        }
        return format_command(output, "PSUBSCRIBE", pattern);
    }

    T pattern;
};

template<typename T>
struct PUNSUBSCRIBE : public subscriber_command
{
    PUNSUBSCRIBE() : pattern(T()) {}

    virtual std::error_code write_command(stream& output) const override
    {
        return format_command(output, "PUNSUBSCRIBE", pattern);
    }

    T pattern;
};

template<typename T>
struct UNSUBSCRIBE : public subscriber_command
{
    UNSUBSCRIBE() : channel(T()) {}

    virtual std::error_code write_command(stream& output) const override
    {
        return format_command(output, "UNSUBSCRIBE", channel);
    }

    T channel;
};

template<typename T>
struct SUBSCRIBE : public subscriber_command
{
    SUBSCRIBE() : channel(T()) {}

    virtual std::error_code write_command(stream& output) const override
    {
        if (count_element(channel) == 0) {
            return error::invalid_command_format;
        }
        return format_command(output, "SUBSCRIBE", channel);
    }

    T channel;
};

template<typename T>
struct PUBLISH : public single_key_command
{
    static_assert(is_single_element_type<T>::value, "T should be represented in a single string.");

    PUBLISH() : message(T()) {}

    virtual std::error_code write_command(stream& output) const override
    {
        return format_command(output, "PUBLISH", key, message);
    }
    
    T message;
};

// scripting-related commands
// TODO

} // namespace "redis"

#endif // REDIS_COMMAND_H
