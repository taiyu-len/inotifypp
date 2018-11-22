#include "inotifypp/event_buffer.hpp"
#include "inotifypp/event.hpp"
#include <cstring>
#include <unistd.h>

namespace inotifypp
{

auto event_buffer::top() const noexcept -> inotify_event const*
{
	return reinterpret_cast<inotify_event const*>(_start);
}

auto event_buffer::pop() noexcept -> inotify_event const*
{
	auto event = top();
	_start += sizeof(*event) + event->len;
	return event;
}

auto event_buffer::try_push(event const& e) noexcept -> bool
{
	size_t size = sizeof(inotify_event) + e.len();
	if (available() < size)
	{
		return false;
	}
	inotify_event ie = {
		e.wd(),
		e.mask(),
		e.cookie(),
		e.len()
	};
	std::memcpy(_end, &ie, sizeof(ie));
	_end += sizeof(ie);
	std::memcpy(_end, e.name(), e.len());
	_end += e.len();
	return true;
}

} // inotifypp
