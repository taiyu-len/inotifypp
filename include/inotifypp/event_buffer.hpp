#ifndef INOTIFYPP_WATCH_BUFFER_HPP
#define INOTIFYPP_WATCH_BUFFER_HPP
#include "inotifypp/fwd.hpp"
#include <cstddef>
#include <iterator>
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

	auto data() const noexcept -> std::byte*  { return _first; }
	auto size() const noexcept -> size_t      { return _last - _first; }
	auto remaining() const noexcept -> size_t { return _end - _start; }
	auto available() const noexcept -> size_t { return _last - _end; }

	void grow(size_t x) noexcept { _end += x; }

	// returns the current event. pop will update to next event.
	auto top() const noexcept -> inotify_event const*;
	auto pop() noexcept -> inotify_event const*;

	// tries to push new event into the buffer, returning false if there
	// is no room
	auto try_push(event const&) noexcept -> bool;

	void clear() noexcept { _start = _end = _first; }
private:
	// [first, start, end, last]
	// [first,             last]  = Total memory available for reading
	// [first, start]             = Events already processed.
	//        [start, end]        = Remaining events to be processed
	//               [end, last]  = space in buffer for pushing events
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
