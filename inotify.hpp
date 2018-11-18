#include <cstddef>
#include <cstring>
#include <limits.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <sys/inotify.h>
#include <system_error>
#include <unistd.h>

namespace inotify
{

struct instance;
struct watch_item;
struct event_buffer;
struct event_ref;
struct event;

using watch_descriptor = int;
using bitmask = uint32_t;
using cookie_t = uint32_t;

static const
size_t min_buffer_size = sizeof(struct inotify_event) + NAME_MAX + 1;

// Buffer used to store watch events into
struct event_buffer
{
	template<typename T>
	event_buffer(T& x);
	event_buffer() = default;

	auto data() -> std::byte *;
	auto size() -> size_t;
	auto available() -> size_t;

	// returns the next event, 
	auto next() -> inotify_event const*;
	// read more from fd if no more events buffered
	auto read(int fd) -> std::error_code;

	// try pushing an event onto the end of the buffer manually,
	// returns false if no room available.
	auto push(event const&) -> bool;
	auto push(event &&) -> bool;
private:
	std::byte *_first = nullptr;
	std::byte *_start = nullptr;
	std::byte *_end   = nullptr;
	std::byte *_last  = nullptr;
};

/** Creates an inotify instance,
 * allows adding paths to the watch list,
 * synchronously and asynchronously watching for new events.
 */
struct instance
{
	template<typename T>
	instance(T &x);

	~instance() noexcept;
	instance(instance &&) noexcept;
	auto operator=(instance&&) noexcept -> instance&;
	auto add_watch(char const*, bitmask) const -> watch_item;
	auto add_watch(std::string const&, bitmask) const -> watch_item;
	auto watch() -> event_ref;
	auto watch(std::error_code&) noexcept -> event_ref;

private:
	auto read_events() noexcept -> std::error_code;
	auto next_event() noexcept -> inotify_event const*;

	int _fd;
	event_buffer _buffer;
};

/// watch item that will remove watch on destruction
struct watch_item
{
	watch_item(watch_descriptor wd, int fd) noexcept;
	~watch_item() noexcept;

	auto wd() const noexcept -> watch_descriptor { return _wd; }
	auto fd() const noexcept -> int { return _fd; }
private:
	watch_descriptor _wd;
	int _fd;
};

/// event proxy wrapper over inotify_event
struct event_ref
{
	event_ref(inotify_event const* e):_event(e) {};
	event_ref() = default;

	auto wd() const noexcept -> watch_descriptor;
	auto mask() const noexcept -> bitmask;
	auto cookie() const noexcept -> cookie_t;
	auto len() const noexcept -> uint32_t;
	auto name() const noexcept -> const char*;
private:
	inotify_event const* _event = nullptr;
};

/// value representing an inotify_event
struct event
{
	event(inotify_event const*);
	event(event_ref);

	auto wd() const noexcept -> watch_descriptor;
	auto mask() const noexcept -> bitmask;
	auto cookie() const noexcept -> cookie_t;
	auto len() const noexcept -> uint32_t;
	auto name() const noexcept -> const char*;
private:
	watch_descriptor _wd;
	bitmask          _mask;
	cookie_t         _cookie;
	uint32_t         _len;
	std::string      _name;
};

#define IN_EVENT_TEST(NAME, VALUE) \
inline auto NAME(event_ref    x) -> bool { return x.mask() & VALUE; } \
inline auto NAME(event const& x) -> bool { return x.mask() & VALUE; }

IN_EVENT_TEST(in_access,        IN_ACCESS);
IN_EVENT_TEST(in_attrib,        IN_ATTRIB);
IN_EVENT_TEST(in_close_write,   IN_CLOSE_WRITE);
IN_EVENT_TEST(in_close_nowrite, IN_CLOSE_NOWRITE);
IN_EVENT_TEST(in_create,        IN_CREATE);
IN_EVENT_TEST(in_delete,        IN_DELETE);
IN_EVENT_TEST(in_delete_self,   IN_DELETE_SELF);
IN_EVENT_TEST(in_modify,        IN_MODIFY);
IN_EVENT_TEST(in_move_self,     IN_MOVE_SELF);
IN_EVENT_TEST(in_moved_from,    IN_MOVED_FROM);
IN_EVENT_TEST(in_moved_to,      IN_MOVED_TO);
IN_EVENT_TEST(in_open,          IN_OPEN);

IN_EVENT_TEST(in_move,          IN_MOVE);
IN_EVENT_TEST(in_close,         IN_CLOSE);

IN_EVENT_TEST(in_dont_follow,   IN_DONT_FOLLOW);
IN_EVENT_TEST(in_excl_unlink,   IN_EXCL_UNLINK);
IN_EVENT_TEST(in_mask_add,      IN_MASK_ADD);
IN_EVENT_TEST(in_oneshot,       IN_ONESHOT);
IN_EVENT_TEST(in_onlydir,       IN_ONLYDIR);

IN_EVENT_TEST(in_ignored,       IN_IGNORED);
IN_EVENT_TEST(in_isdir,         IN_ISDIR);
IN_EVENT_TEST(in_q_overflow,    IN_Q_OVERFLOW);
IN_EVENT_TEST(in_unmount,       IN_UNMOUNT);

#undef IN_EVENT_TEST
namespace detail
{
inline auto errno_code() -> std::error_code
{
	auto e = std::errc(errno);
	return std::make_error_code(e);
}
}

template<typename T>
instance::instance(T &x)
: _fd(inotify_init1(IN_CLOEXEC))
, _buffer(x)
{
	if (this->_fd == -1)
	{
		throw detail::errno_code();
	}
}

instance::~instance() noexcept
{
	if (this->_fd != -1)
	{
		close(this->_fd);
	}
}

instance::instance(instance&& other) noexcept
: _fd(other._fd)
, _buffer(other._buffer)
{
	other._fd = -1;
	other._buffer = event_buffer{};
}

auto instance::operator=(instance&& other) noexcept -> instance&
{
	if (this->_fd != -1)
	{
		close(this->_fd);
	}
	this->_fd = std::exchange(other._fd, -1);
	return *this;
}

auto instance::add_watch(char const* str, bitmask m) const -> watch_item
{
	auto wd = inotify_add_watch(this->_fd, str, m);
	if (wd == -1)
	{
		throw detail::errno_code();
	}
	return watch_item{ wd, this->_fd };
}

auto instance::add_watch(std::string const& str, bitmask m) const -> watch_item
{
	return add_watch(str.c_str(), m);
}

auto instance::watch() -> event_ref
{
	std::error_code ec;
	auto er = this->watch(ec);
	if (ec) throw ec;
	return er;
}

auto instance::watch(std::error_code &ec) noexcept -> event_ref
{
	if (ec = this->_buffer.read(this->_fd))
	{
		return event_ref{};
	}
	return this->_buffer.next();
}

template<typename T>
event_buffer::event_buffer(T &x)
: _first(x.data())
, _start(_first)
, _end(_first)
, _last(x.data() + x.size())
{}

auto event_buffer::data() -> std::byte *
{
	return _first;
}

auto event_buffer::size() -> size_t
{
	return _last - _first;
}

auto event_buffer::available() -> size_t
{
	return _last - _end;
}

auto event_buffer::next() -> inotify_event const*
{
	auto event = reinterpret_cast<inotify_event const*>(this->_start);
	this->_start += sizeof(*event) + event->len;
	return event;
}

auto event_buffer::read(int fd) -> std::error_code
{
	// TODO use asio to handle reads/async reads on file descriptor
	if (this->_start < this->_end)
	{
		return {};
	}
	this->_start = this->_end = this->_first;
	auto bytes_read = ::read(fd, this->_first, this->available());
	if (bytes_read == -1)
	{
		return detail::errno_code();
	}
	this->_end += bytes_read;
	return {};
}

auto event_buffer::push(event const& e) -> bool
{
	size_t size = sizeof(struct inotify_event) + e.len();
	if (this->available() < size)
	{
		return false;
	}
	inotify_event ie = {
		e.wd(), e.mask(), e.cookie(), e.len()
	};
	std::memcpy(_end, &ie, sizeof(ie));
	_end += sizeof(ie);
	std::memcpy(_end, e.name(), e.len());
	_end += e.len();
	return true;
}

watch_item::watch_item(watch_descriptor wd, int fd) noexcept
: _wd(wd), _fd(fd)
{}

watch_item::~watch_item() noexcept
{
	inotify_rm_watch(this->_fd, this->_wd);
}

#define IN_EVENT_FUNC(T, FN, TYPE, NAME) \
auto T::FN() const noexcept -> TYPE \
{ return this->NAME; } \

IN_EVENT_FUNC(event_ref, wd    , watch_descriptor, _event->wd);
IN_EVENT_FUNC(event_ref, mask  , bitmask         , _event->mask);
IN_EVENT_FUNC(event_ref, cookie, cookie_t        , _event->cookie);
IN_EVENT_FUNC(event_ref, len   , uint32_t        , _event->len);
IN_EVENT_FUNC(event_ref, name  , const char *    , _event->name);

IN_EVENT_FUNC(event, wd    , watch_descriptor, _wd);
IN_EVENT_FUNC(event, mask  , bitmask         , _mask);
IN_EVENT_FUNC(event, cookie, cookie_t        , _cookie);
IN_EVENT_FUNC(event, len   , uint32_t        , _len);
IN_EVENT_FUNC(event, name  , const char *    , _name.c_str());

#undef IN_EVENT_FUNC
}

