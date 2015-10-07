#include "asio_adaptor.h"

#include <vector>
#include <string>
#include <memory>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/system/error_code.hpp>

#include "redis_base.h"

namespace {

boost::asio::io_service io_service(8);

} // the end of anonymous namespace

asio_stream_adaptor::asio_stream_adaptor(size_t initial_buffer_size)
	: read_buffer_(initial_buffer_size), write_buffer_(initial_buffer_size), socket_(io_service)
{
	reset();
}

// redis::stream interface implementation
bool asio_stream_adaptor::close()
{
	socket_.close(err_code_);
	reset();

	return err_code_ ? false : true;
}

bool asio_stream_adaptor::is_open() const
{
	return socket_.is_open();
}

// redis::stream input interface implementation
size_t asio_stream_adaptor::available() const
{
	return to_be_read_.size() + socket_.available();
}

redis::const_buffer_view asio_stream_adaptor::peek(size_t n)
{
	auto result = ensure_available_buffer(std::min(n, available()));
	return result.first;
}

redis::const_buffer_view asio_stream_adaptor::read(size_t n)
{
	auto result = ensure_available_buffer(n);
	if (result.first.valid()) {
		to_be_read_ = result.second;
	}
	return result.first;
}

size_t asio_stream_adaptor::skip(size_t n)
{
	return read(n).size();
}

// utility functions for read interface
std::pair<redis::buffer_view, redis::buffer_view> asio_stream_adaptor::ensure_available_buffer(size_t at_least)
{
	if (to_be_read_.size() < at_least) {
		auto read_result = read_from_socket(at_least - to_be_read_.size());
		if (!read_result) {
			return redis::buffer_view().split(0); // returns invalid buffer
		}
	}

	assert(read_range_check());
	assert(to_be_read_.size() >= at_least);
	return to_be_read_.split(at_least);
}

bool asio_stream_adaptor::read_from_socket(size_t at_least)
{
	if (to_be_read_.size() == 0 || unused_read_buffer().size() < at_least) {
		// when the remaining data size is zero, move read view to head of the buffer to reduce the number of copying
		move_and_ensure_read_buffer(at_least);
	}

	boost::system::error_code ec;
	size_t read_byte = 0;

	while (read_byte < at_least) {
		auto unused = unused_read_buffer();
		auto result = socket_.read_some(boost::asio::buffer(unused.data(), unused.size()), ec);
		if (ec || result == 0) {
			err_code_ = ec;
			return false;
		}

		read_byte += result;

		to_be_read_ = redis::buffer_view(to_be_read_.begin(), to_be_read_.size() + result);
		assert(read_range_check());		
	}

	return true;
}
	
redis::buffer_view asio_stream_adaptor::unused_read_buffer()
{
	return redis::buffer_view(to_be_read_.end(), read_buffer_.data() + read_buffer_.size());
}
	
void asio_stream_adaptor::move_and_ensure_read_buffer(size_t at_least)
{
	bool need_to_resize = read_buffer_.size() - to_be_read_.size() < at_least;
	size_t available_size = to_be_read_.size();

	if (need_to_resize) {
		std::vector<char> swapped(available_size + at_least);
		std::copy(to_be_read_.begin(), to_be_read_.end(), swapped.begin());
		read_buffer_.swap(swapped);
	} else {
		std::copy(to_be_read_.begin(), to_be_read_.end(), read_buffer_.begin());			
	}

	to_be_read_ = redis::buffer_view(read_buffer_.data(), available_size);
	assert(read_range_check());
}

bool asio_stream_adaptor::read_range_check() const
{
	return to_be_read_.valid() &&
		(to_be_read_.begin() >= read_buffer_.data()) &&
		(to_be_read_.end() <= read_buffer_.data() + read_buffer_.size());
}

// redis::stream output interface implementation
bool asio_stream_adaptor::flush()
{
	boost::system::error_code ec;
	auto size = boost::asio::write(socket_, boost::asio::buffer(to_be_written_.data(), to_be_written_.size()), boost::asio::transfer_all(), ec);

	assert(size == to_be_written_.size()); // because of transfer_all

	if (ec || size < to_be_written_.size()) {
		err_code_ = ec;
		return false;
	} else {
		to_be_written_ = redis::buffer_view(write_buffer_.data(), 0u);
		assert(write_range_check());
		return true;
	}
}

bool asio_stream_adaptor::write(redis::const_buffer_view input)
{
	if (input.size() > unused_write_buffer().size()) {
		write_buffer_.resize(std::max(to_be_written_.size() + input.size(), write_buffer_.size() * 2));
		to_be_written_ = redis::buffer_view(write_buffer_.data(), to_be_written_.size());
	}

	assert(input.size() <= unused_write_buffer().size());
	std::copy(input.begin(), input.end(), unused_write_buffer().begin());
	to_be_written_ = redis::buffer_view(write_buffer_.data(), to_be_written_.size() + input.size());

	return true;
}

bool asio_stream_adaptor::write_range_check() const
{
	return to_be_written_.valid() &&
		(to_be_written_.begin() >= write_buffer_.data()) &&
		(to_be_written_.end() <= write_buffer_.data() + write_buffer_.size());
}

redis::buffer_view asio_stream_adaptor::unused_write_buffer()
{
	return redis::buffer_view(to_be_written_.end(), write_buffer_.data() + write_buffer_.size());
}

// asio_stream_adaptor member functions
void asio_stream_adaptor::reset()
{
	to_be_read_ = redis::buffer_view(read_buffer_.data(), 0u);
	to_be_written_ = redis::buffer_view(write_buffer_.data(), 0u);
	move_and_ensure_read_buffer(0);
}

bool asio_stream_adaptor::connect(const std::string& ip, uint16_t port, int32_t time_out)
{
	if (socket_.is_open()) {
		err_code_ = boost::asio::error::already_connected;
		return false;
	}

	if (!connect_with_time_out(ip, port, time_out)) {
		return false;
	}
		
	if (::setsockopt(socket_.native(), SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&time_out), sizeof(time_out)) == SOCKET_ERROR ||
		::setsockopt(socket_.native(), SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&time_out), sizeof(time_out)) == SOCKET_ERROR) {
		err_code_.assign(::WSAGetLastError(), boost::system::system_category());
		close();
		return false;
	}
	return true;
}

bool asio_stream_adaptor::connect_with_time_out(const std::string& ip, uint16_t port, int32_t time_out)
{
	using boost::asio::ip::tcp;
	using boost::asio::deadline_timer;

	tcp::resolver resolver(io_service);
	tcp::resolver::query query(ip, boost::lexical_cast<std::string>(port)); // v6?

	boost::system::error_code err;

	auto iterator = resolver.resolve(query, err);
	if (err) {
		err_code_ = err;
		return false;
	}

	deadline_timer t(socket_.get_io_service());

	bool error_occurred = false;

	t.expires_from_now(boost::posix_time::seconds(time_out));
	t.async_wait([this, &error_occurred](const boost::system::error_code& ec) {
		if (ec != boost::asio::error::operation_aborted) {
			err_code_ = boost::asio::error::timed_out;
			error_occurred = true;
			close();
		}
	});

	socket_.async_connect(iterator->endpoint(), [this, &t, &error_occurred](const boost::system::error_code& ec) {
		if (!socket_.is_open()) { // time out
			// already handled in the timer callback
		} else if (!ec) {  // no error
			t.cancel();				
		} else { // connection error
			err_code_ = ec;
			error_occurred = true;
			close();
		}
	});

	while(socket_.get_io_service().run_one()) {}

	return !error_occurred;
}

/*
const int FLUSH = 32768;
const int WRITE = 256;

char input[FLUSH*2];

void do_something()
{
	for (int i = 0; i < FLUSH*2; i++) {
		input[i] = i % WRITE;
	}

	const int thread_count = 100;
	boost::thread t[thread_count];

	for (int i = 0; i < thread_count; i++) {
		t[i] = boost::thread([=]{ echo_test(i); } );
	}

	for (int i = 0; i < thread_count; i++) {
		t[i].join();
	}
}

void echo_test(int thread_num)
{
	// test with echo server
	asio_stream_adaptor stream;

	//stream.connect("www.google.com", 81); // timeout test
	stream.connect("localhost", "50000");
	//stream.connect("10.3.1.68", "50000");

	int count = 0;
	__int64 total_written = 0;
	__int64 total_read = 0;

	for (;;) {
		int flush_size = rand() % (FLUSH-WRITE);
		int written_byte = 0;

		while (written_byte < flush_size) {
			int write_size = rand() % WRITE;
			stream->write(redis::const_buffer_view(input+(total_written % FLUSH), write_size));
			written_byte += write_size;
			total_written += write_size;
		}
		stream->flush();

		while (total_read < total_written) {
			int read_size = rand() % WRITE;
			if (total_read + read_size > total_written) {
				break;
			}
			auto result = stream->read(read_size);
			assert(result.size() == read_size);

			assert(std::equal(result.begin(), result.end(), input+(total_read % FLUSH)));
			total_read += read_size;
		}

		if (++count % 1000 == 0) {
			printf("[%d] %d - %lld / %lld\n", thread_num, count, total_written, total_read);
		}
	}
}
*/