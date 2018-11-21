#ifndef INOTIFYPP_FWD_HPP
#define INOTIFYPP_FWD_HPP
#include <boost/asio/posix/stream_descriptor.hpp>
#include <cstddef>
#include <limits.h>
#include <sys/inotify.h>

namespace inotifypp
{
static const
size_t min_buffer_size = sizeof(struct inotify_event) + NAME_MAX + 1;

struct instance;
struct watch_item;
struct event_buffer;
struct event_ref;
struct event;

using stream_descriptor = boost::asio::posix::stream_descriptor;
using watch_descriptor = int;
using mask_t = uint32_t;
using cookie_t = uint32_t;
} // inotifypp

#endif // INOTIFYPP_FWD_HPP
