#include "inotifypp/instance.hpp"
#include "inotifypp/watch_item.hpp"
#include "inotifypp/event.hpp"
#include <errno.h>
#include <unistd.h>
#include <utility>

namespace inotifypp
{
instance::instance(event_buffer x)
: _fd(inotify_init1(IN_CLOEXEC))
, _buffer(x)
{
	if (_fd == -1)
	{
		throw std::make_error_code(std::errc(errno));
	}
}

instance::~instance() noexcept
{
	if (_fd != -1)
	{
		::close(_fd);
	}
}

instance::instance(instance&& x) noexcept
: _fd(x._fd)
, _buffer(x._buffer)
{
	x._fd = -1;
	x._buffer = event_buffer{};
}

auto instance::operator=(instance&& x) noexcept -> instance&
{
	if (_fd != -1)
	{
		::close(_fd);
	}
	_fd = std::exchange(x._fd, -1);
	x._buffer = event_buffer{};
	return *this;
}

auto instance::add_watch(char const* str, mask_t m) const -> watch_item
{
	auto wd = inotify_add_watch(_fd, str, m);
	if (wd == -1)
	{
		throw std::make_error_code(std::errc(errno));
	}
	return watch_item{ wd, _fd };
}

auto instance::add_watch(std::string const& str, mask_t m) const -> watch_item
{
	return add_watch(str.c_str(), m);
}

auto instance::watch() -> event_ref
{
	std::error_code ec;
	auto er = watch(ec);
	if (ec)
	{
		throw ec;
	}
	return er;
}

auto instance::watch(std::error_code &ec) noexcept -> event_ref
{
	if ((ec = _buffer.read(_fd)))
	{
		return event_ref{};
	}
	return _buffer.next();
}

} // inotifypp

