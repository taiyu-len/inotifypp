#ifndef INOTIFYPP_WATCH_ITEM_HPP
#define INOTIFYPP_WATCH_ITEM_HPP
#include "inotifypp/fwd.hpp"

namespace inotifypp
{
/** RAII clean that removes watch on destruction. */
struct watch_item
{
	watch_item(watch_descriptor wd, int fd) noexcept
	: _wd(wd)
	, _fd(fd) {};

	~watch_item() noexcept {
		inotify_rm_watch(_fd, _wd);
	}

	auto wd() const noexcept -> watch_descriptor {
		return _wd;
	}

	auto fd() const noexcept -> int {
		return _fd;
	}

	/** Call to avoid removing watch on destruction.
	 * useful if the watch lasts as long as instance does */
	void forget() noexcept {
		_wd = _fd = -1;
	}
private:
	watch_descriptor _wd;
	int _fd;
};
} // inotifypp

#endif // INOTIFYPP_WATCH_ITEM_HPP
