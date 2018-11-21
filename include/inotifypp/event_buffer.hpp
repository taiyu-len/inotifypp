#ifndef INOTIFYPP_WATCH_BUFFER_HPP
#define INOTIFYPP_WATCH_BUFFER_HPP
#include "inotifypp/fwd.hpp"
#include <boost/asio/async_result.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/system/error_code.hpp>
#include <cstddef>
#include <iterator>
#include <system_error>
#include <utility>

namespace inotifypp
{
struct event_buffer
{
	template<
		class T, class =
		decltype(std::declval<T>().data() + std::declval<T>().size())>
	event_buffer(T& x);
	event_buffer() = default;

	auto data() const noexcept -> std::byte*;
	auto size() const noexcept -> size_t;
	auto available() const noexcept -> size_t;

	// returns the next event.
	auto top() const -> inotify_event const*;
	auto pop() -> inotify_event const*;

	auto read(stream_descriptor& sd) -> std::error_code;

	template<typename H>
	void async_read(stream_descriptor& sd, H&& handler);

	// tries to push new event into the buffer, returning false if there
	// is no room
	auto try_push(event const&) -> bool;

	void clear();
private:
	std::byte *_first = nullptr;
	std::byte *_start = nullptr;
	std::byte *_end   = nullptr;
	std::byte *_last  = nullptr;
};

// TODO, ensure proper alignment
template<class T, class>
event_buffer::event_buffer(T &x)
: _first(std::data(x))
, _start(_first)
, _end(_first)
, _last(std::data(x) + std::size(x))
{}

template<typename H>
void event_buffer::async_read(stream_descriptor& sd, H&& h)
{
	auto f = [this, &sd, h = std::forward<H>(h)] (
		boost::system::error_code const& ec, size_t bytes_read)
	{
		if (ec) h(std::make_error_code(std::errc(ec.value())), {});
		this->_end += bytes_read;
		while (this->_start < this->_end)
		{
			h({}, this->pop());
		}
		this->clear();
		this->async_read(sd, std::move(h));
	};
	auto b = boost::asio::mutable_buffer({_end, available()});
	sd.async_read_some(b, std::move(f));
}

} // inotifypp

#endif // INOTIFYPP_WATCH_BUFFER_HPP
