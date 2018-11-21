#include "inotifypp/event.hpp"
namespace inotifypp
{
event::event(event_ref x)
: _wd(x.wd())
, _mask(x.mask())
, _cookie(x.cookie())
, _name(x.name(), x.len())
{
	const auto a = alignof(inotify_event);
	const auto e = len() % a;
	// pad with null bytes until suitably aligned
	if (e != 0) {
		_name.resize(len() + a-e, '\0');
	}
}

event::event(inotify_event x, const char* s)
: _wd(x.wd)
, _mask(x.mask)
, _cookie(x.cookie)
, _name(s)
{
	const auto a = alignof(inotify_event);
	const auto e = len() % a;
	if (e != 0) {
		_name.resize(len() + a-e, '\0');
	}
}


event::event(inotify_event x, std::string s)
: _wd(x.wd)
, _mask(x.mask)
, _cookie(x.cookie)
, _name(std::move(s))
{
	const auto a = alignof(inotify_event);
	const auto e = len() % a;
	if (e != 0) {
		_name.resize(len() + a-e, '\0');
	}
}
} // inotifypp
