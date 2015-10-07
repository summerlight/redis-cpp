#ifndef REDIS_ASIO_ADAPTOR_H
#define REDIS_ASIO_ADAPTOR_H

#include <vector>
#include <memory>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/system/error_code.hpp>

#include "redis_base.h"

// thread-safety : safe in distinct, not safe in shared
struct asio_stream_adaptor : public redis::stream
{
public:
	asio_stream_adaptor(size_t initial_buffer_size = 16384);

	~asio_stream_adaptor()
	{
	}

	// redis::stream interface implementation
	virtual bool close() override;
	virtual bool is_open() const override;

	// redis::stream input interface implementation
	virtual size_t available() const override;
	virtual redis::const_buffer_view peek(size_t n) override;
	virtual redis::const_buffer_view read(size_t n) override;
	virtual size_t skip(size_t n) override;

	// redis::stream output interface implementation
	virtual bool flush() override;
	virtual bool write(redis::const_buffer_view input) override;

	// asio_stream_adaptor member functions
	bool connect(const std::string& ip, uint16_t port, int32_t time_out = 5);
	boost::system::error_code stream_error() const
	{
		return err_code_;
	}

private:
	// utility functions
	void reset();

	std::pair<redis::buffer_view, redis::buffer_view> ensure_available_buffer(size_t at_least);
	bool read_from_socket(size_t at_least);		
	void move_and_ensure_read_buffer(size_t at_least);

	bool read_range_check() const;
	bool write_range_check() const;

	redis::buffer_view unused_read_buffer();
	redis::buffer_view unused_write_buffer();

	bool connect_with_time_out(const std::string& ip, uint16_t port, int32_t time_out);

private:
	std::vector<char> read_buffer_;
	std::vector<char> write_buffer_;
	redis::buffer_view to_be_read_;
	redis::buffer_view to_be_written_;

	boost::asio::ip::tcp::socket socket_;
	boost::system::error_code err_code_;
};

#endif // REDIS_ASIO_ADAPTOR_H