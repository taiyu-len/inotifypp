#ifndef INOTIFYPP_INSTANCE_HPP
#define INOTIFYPP_INSTANCE_HPP
#include "inotifypp/fwd.hpp"
#include "inotifypp/event_buffer.hpp"
#include <string>
#include <sys/inotify.h>
#include <system_error>

namespace inotifypp
{
struct instance
{
	instance(event_buffer x);
	~instance() noexcept;
	instance(instance &&) noexcept;
	auto operator=(instance&&) noexcept -> instance&;
	auto add_watch(char const*, mask_t) const -> watch_item;
	auto add_watch(std::string const&, mask_t) const -> watch_item;
	auto watch() -> event_ref;
	auto watch(std::error_code&) noexcept -> event_ref;

private:
	int _fd;
	event_buffer _buffer;
};
} // inotifypp

#endif // INOTIFYPP_INSTANCE_HPP
