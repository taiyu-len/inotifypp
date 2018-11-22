#include "inotifypp/event.hpp"
#include "inotifypp/instance.hpp"
#include "inotifypp/watch_item.hpp"
#include <errno.h>
#include <unistd.h>
#include <utility>

namespace inotifypp
{
instance::instance(boost::asio::io_context& io, event_buffer x)
: _sd(io)
, _buffer(x)
{
	auto fd = inotify_init1(IN_CLOEXEC);
	if (fd == -1)
	{
		throw std::make_error_code(std::errc(errno));
	}
	_sd.assign(fd);
}

instance::~instance() noexcept
{
	_sd.close();
}

instance::instance(instance&& x) noexcept
: _sd(std::move(x._sd))
, _buffer(x._buffer)
{
	x._buffer = event_buffer{};
}

auto instance::operator=(instance&& x) noexcept -> instance&
{
	_sd = std::move(x._sd);
	x._buffer = event_buffer{};
	return *this;
}

auto instance::add_watch(char const* str, mask_t m) -> watch_item
{
	auto wd = inotify_add_watch(_sd.native_handle(), str, m);
	if (wd == -1)
	{
		throw std::make_error_code(std::errc(errno));
	}
	return watch_item{ wd, _sd.native_handle() };
}

auto instance::add_watch(std::string const& str, mask_t m) -> watch_item
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
	// If there are no events in buffer, read in more.
	if (_buffer.remaining() == 0)
	{
		_buffer.clear();
		auto boost_ec = boost::system::error_code{};
		auto buffer = boost::asio::buffer(_buffer.data(), _buffer.size());
		auto bytes_read = _sd.read_some(buffer, boost_ec);
		ec = std::make_error_code(std::errc(boost_ec.value()));
		if (ec) return {};
		_buffer.grow(bytes_read);
	}
	// get next event from buffer
	return _buffer.pop();
}

} // inotifypp

