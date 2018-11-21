#ifndef INOTIFYPP_WATCH_BUFFER_HPP
#define INOTIFYPP_WATCH_BUFFER_HPP
#include "inotifypp/fwd.hpp"
#include <boost/asio/async_result.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/post.hpp>
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

template<typename EventHandler>
// EventHandler: x(std::error_code, inotify_event const*);
// will spawn an async job on sd's context that calls h once
void event_buffer::async_read(stream_descriptor& sd, EventHandler&& h)
{
	// there are unhandled elements in the buffer
	if (this->_start < this->_end)
	{
		auto f = [this, h = std::forward<EventHandler>(h)] () mutable
		{
			h({}, this->pop());
		};
		boost::asio::post(sd.get_executor(), std::move(f));
		return;
	}
	// else we need to read more events, so we clear the buffer,
	// create a read handler, and call it asynchronously
	this->clear();
	auto f = [this, h = std::forward<EventHandler>(h)] (
		boost::system::error_code const& ec,
		size_t bytes_read) mutable
	{
		if (ec)
		{
			h(std::make_error_code(std::errc(ec.value())), {});
		}
		else
		{
			this->_end += bytes_read;
			h({}, this->pop());
		}
	};
	auto b = boost::asio::mutable_buffer({data(), size()});
	sd.async_read_some(b, std::move(f));
}

} // inotifypp

#endif // INOTIFYPP_WATCH_BUFFER_HPP
