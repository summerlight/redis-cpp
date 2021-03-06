#include "error.h"

namespace redis
{

namespace error
{

std::error_code make_error_code(error_t e)
{
    return std::error_code(static_cast<int>(e), category());
}

std::error_condition make_error_condition(error_t e)
{
    return std::error_condition(static_cast<int>(e), category());
}

} // namespace "redis::error"

const char* error_category::name() const noexcept
{
    return "redis_error";
}

std::string error_category::message(int ev) const noexcept
{
    switch(ev) {
    case error::error_reply:
        return "redis server returns error reply - check handler.error_info";
    case error::ill_formed_reply:
        return "reply string is ill-formed";
    case error::invalid_command_format:
        return "invalid command format";
    case error::stream_not_initialized:
        return "stream is not initialized";
    case error::stream_error:
        return "error in while processing stream - check your stream object for detailed information";
    case error::handler_error:
        return "given reply handler object failed to handle reply";
    case error::subscriber_cmd_error:
        return "subscriber command should only be used in compatible session object";
    default:
        return "unknown error code - error code object may be corrupted";
    }
}

namespace {
thread_local error_category error_category_instance;
} // the end of anonmymous namespace

const std::error_category& category()
{	
    return error_category_instance;
}



} // namespace "redis"