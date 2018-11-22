#ifndef INOTIFYPP_INSTANCE_HPP
#define INOTIFYPP_INSTANCE_HPP
#include "inotifypp/event_buffer.hpp"
#include "inotifypp/fwd.hpp"
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/system/error_code.hpp>
#include <string>
#include <sys/inotify.h>
#include <system_error>

namespace inotifypp
{
struct instance
{
	instance(boost::asio::io_context &, event_buffer x);
	~instance() noexcept;
	instance(instance &&) noexcept;
	auto operator=(instance&&) noexcept -> instance&;
	auto buffer() noexcept -> event_buffer& { return _buffer; };
	auto add_watch(char const*, mask_t) -> watch_item;
	auto add_watch(std::string const&, mask_t) -> watch_item;
	auto watch() -> event_ref;
	auto watch(std::error_code&) noexcept -> event_ref;

	template<typename H>
	void async_watch(H && handler);
private:
	stream_descriptor _sd;
	event_buffer _buffer;
};

template<typename EventHandler>
// EventHandler: x(std::error_code, event_ref);
// will spawn an async job in sd's executor that calls h once
void instance::async_watch(EventHandler && handler)
{
	auto fn = [this, handler = std::forward<EventHandler>(handler)] (
		boost::system::error_code const& ec,
		size_t bytes_read) mutable
	{
		if (ec)
		{
			handler(std::make_error_code(std::errc(ec.value())), {});
		}
		else
		{
			_buffer.grow(bytes_read);
			handler({}, event_ref(_buffer.pop()));
		}
	};
	// no more events
	if (_buffer.remaining() == 0)
	{
		_buffer.clear();
		auto b = boost::asio::buffer(_buffer.data(), _buffer.size());
		_sd.async_read_some(b, std::move(fn));
	}
	// existing events
	else
	{
		auto f = [fn=std::move(fn)] () mutable
		{
			fn({}, 0);
		};
		boost::asio::post(_sd.get_executor(), std::move(f));
	}
}
} // inotifypp

#endif // INOTIFYPP_INSTANCE_HPP
