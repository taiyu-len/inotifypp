#ifndef INOTIFYPP_INSTANCE_HPP
#define INOTIFYPP_INSTANCE_HPP
#include "inotifypp/event_buffer.hpp"
#include "inotifypp/fwd.hpp"
#include <boost/asio/io_context.hpp>
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
	auto add_watch(char const*, mask_t) -> watch_item;
	auto add_watch(std::string const&, mask_t) -> watch_item;
	auto watch() -> event_ref;
	auto watch(std::error_code&) noexcept -> event_ref;

private:
	stream_descriptor _sd;
	event_buffer _buffer;
};
} // inotifypp

#endif // INOTIFYPP_INSTANCE_HPP
