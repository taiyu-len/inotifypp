#ifndef INOTIFYPP_EVENT_HPP
#define INOTIFYPP_EVENT_HPP
#include "inotifypp/fwd.hpp"
#include <string>

namespace inotifypp
{
struct event_ref
{
	event_ref(inotify_event const* e) : _event(e) {}
	event_ref() = default;

	auto wd()     const noexcept -> watch_descriptor { return _event->wd; }
	auto mask()   const noexcept -> mask_t { return _event->mask; }
	auto cookie() const noexcept -> cookie_t { return _event->cookie; }
	auto len()    const noexcept -> uint32_t { return _event->len; }
	auto name()   const noexcept -> const char* { return len() ? _event->name : ""; }
private:
	const inotify_event* _event = nullptr;
};

struct event
{
	event(event_ref);
	event(inotify_event, const char*);
	event(inotify_event, std::string);

	auto wd()     const noexcept -> watch_descriptor { return _wd; }
	auto mask()   const noexcept -> mask_t { return _mask; }
	auto cookie() const noexcept -> cookie_t { return _cookie; }
	auto len()    const noexcept -> uint32_t { return _name.size(); }
	auto name()   const noexcept -> const char* { return _name.c_str(); }
private:
	watch_descriptor _wd;
	mask_t           _mask;
	cookie_t         _cookie;
	std::string      _name;
};

} // inotifypp

#endif // INOTIFYPP_EVENT_HPP
