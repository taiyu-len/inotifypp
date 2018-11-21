#include "inotifypp/event_buffer.hpp"
#include "inotifypp/event.hpp"
#include <cstring>
#include <errno.h>
#include <system_error>
#include <unistd.h>

namespace inotifypp
{
auto event_buffer::data() const noexcept -> std::byte *
{
	return _first;
}

auto event_buffer::size() const noexcept -> size_t
{
	return _last - _first;
}

auto event_buffer::available() const noexcept -> size_t
{
	return _last - _end;
}

auto event_buffer::next() -> inotify_event const*
{
	auto event = reinterpret_cast<inotify_event const*>(_start);
	_start += sizeof(*event) + event->len;
	return event;
}

auto event_buffer::read(int fd) -> std::error_code
{
	// TODO use asio to handle reads/async reads on file descriptor
	if (_start < _end)
	{
		return {};
	}
	_start = _end = _first;
	auto bytes_read = ::read(fd, _first, available());
	if (bytes_read == -1)
	{
		return std::make_error_code(std::errc(errno));
	}
	_end += bytes_read;
	return {};
}

auto event_buffer::try_push(event const& e) -> bool
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
