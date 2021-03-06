#ifndef REDIS_ERROR_H
#define REDIS_ERROR_H

#include <system_error>

namespace redis
{

namespace error
{

enum error_t
{
    error_reply = 1,
    handler_error,
    subscriber_cmd_error,
    invalid_command_format,
    ill_formed_reply,
    stream_not_initialized,
    stream_error,	
};

std::error_code make_error_code(error_t e);
std::error_condition make_error_condition(error_t e);

} // namespace "redis::error"

class error_category : public std::error_category
{
public:
    virtual const char* name() const noexcept override;
    virtual std::string message(int ev) const noexcept override;
};

const std::error_category& category();

} // namespace "redis"

namespace std
{
    template<>
    struct is_error_code_enum<redis::error::error_t> : public true_type {};
}



#endif // REDIS_ERROR_H