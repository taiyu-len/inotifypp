#include "inotifypp/event_buffer.hpp"
#include "inotifypp/event.hpp"
#include <cstring>
#include <errno.h>
#include <system_error>
#include <unistd.h>
#include <boost/system/error_code.hpp>

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

auto event_buffer::top() const -> inotify_event const*
{
	return reinterpret_cast<inotify_event const*>(_start);
}

auto event_buffer::pop() -> inotify_event const*
{
	auto event = top();
	_start += sizeof(*event) + event->len;
	return event;
}

auto event_buffer::read(stream_descriptor& sd) -> std::error_code
{
	// we still have events unhandled, so dont read.
	if (_start < _end)
	{
		return {};
	}
	this->clear();
	auto ec = boost::system::error_code{};
	auto br = sd.read_some(boost::asio::mutable_buffer{data(), size()}, ec);
	if (! ec)
	{
		_end += br;
	}
	return std::make_error_code(std::errc(ec.value()));;
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

void event_buffer::clear()
{
	_start = _end = _first;
}

} // inotifypp
