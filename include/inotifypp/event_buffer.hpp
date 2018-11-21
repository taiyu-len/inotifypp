#ifndef INOTIFYPP_WATCH_BUFFER_HPP
#define INOTIFYPP_WATCH_BUFFER_HPP
#include "inotifypp/fwd.hpp"
#include <cstddef>
#include <utility>
#include <system_error>
#include <iterator>

namespace inotifypp
{
struct event_buffer
{
	template<
		class T, class =
		decltype(std::declval<T>().data()+std::declval<T>().size())>
	event_buffer(T& x);
	event_buffer() = default;

	auto data() const noexcept -> std::byte *;
	auto size() const noexcept -> size_t;
	auto available() const noexcept -> size_t;

	// returns the next event
	auto next() -> inotify_event const*;

	// read more from fd if no more events buffered
	auto read(int fd) -> std::error_code;

	// try pushing an event onto the end of the buffer manually,
	// returns false if no room available.
	auto try_push(event const&) -> bool;
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
