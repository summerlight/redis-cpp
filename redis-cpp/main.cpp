#include <cstdio>
#include <iostream>
//#include <boost/thread.hpp>

#include "redis.h"
//#include "asio_adaptor.h"
/*
LONG count;

void handle_error(redis::session<asio_stream_adaptor>& sess, std::error_code& err)
{
    if (err) {
        if (err == redis::error::stream_error) {
            std::cout << sess.stream_error().value() << " : "<< sess.stream_error().message() << std::endl;
        }
        std::cout << err.message() << std::endl;
    }
}

template<typename T>
void handle_data(T&& data)
{

}

void do_specific(int thread_num)
{
    redis::session<asio_stream_adaptor> s;
    const size_t data_size = 100;

    s.connect("10.3.1.31", 6379);

    char test[data_size];
    for (int i = 0; i < data_size; i++) {
        test[i] = i;
    }

    for (int i = 0; i < 5000; i++)
    {
        char key[30];
        sprintf(key, "foo%d", thread_num*100000 + i);
        
        {
            redis::SET<redis::const_buffer_view> cmd;
            cmd.key = key;
            cmd.value = redis::const_buffer_view(std::begin(test), std::end(test));

            auto ec = s.request(cmd);
            handle_error(s, ec);
            handle_data(cmd.reply.result);
        }
        
        {
            redis::GET cmd;
            cmd.key = key;

            auto ec = s.request(cmd);
            handle_error(s, ec);
            handle_data(cmd.reply.result);
        }
        
        {
            redis::integer_reply handler;

            auto cmd = redis::make_key_command(key, [&](redis::stream& output) {
                return redis::format_command(output, "DEL", key);
            });

            auto ec = s.request(cmd, handler);
            handle_error(s, ec);
            handle_data(handler.result);
        }
        
        auto old = InterlockedIncrement(&count);
        if (old % 1000 == 0) {
            std::cout << old << std::endl;
        }
    }
    
    s.close();
}

void do_something()
{
    const int thread_count = 100;
    boost::thread t[thread_count];

    for (int i = 0; i < thread_count; i++) {
        t[i] = boost::thread([=]{ do_specific(i); } );
    }

    for (int i = 0; i < thread_count; i++) {
        t[i].join();
    }
}
*/
namespace redis_test
{
    void test_parser();
    void test_command();
    void test_writer();
}

int main()
{
    redis_test::test_parser();
    redis_test::test_writer();
    redis_test::test_command();
    //do_something();
}

/*
"$7\r\nmessage\r\n"
"$8\r\npmessage\r\n"
"$9\r\nsubscribe\r\n"
"$11\r\nunsubscribe\r\n"
"$10\r\npsubscribe\r\n"
"$12\r\npunsubscribe\r\n"
*/