#ifndef INOTIFYPP_WATCH_BUFFER_HPP
#define INOTIFYPP_WATCH_BUFFER_HPP
#include "inotifypp/fwd.hpp"
#include <boost/asio/posix/stream_descriptor.hpp>
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

} // inotifypp

#endif // INOTIFYPP_WATCH_BUFFER_HPP
