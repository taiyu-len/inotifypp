#ifndef INOTIFYPP_EVENT_TEST_HPP
#define INOTIFYPP_EVENT_TEST_HPP
#include "inotifypp/fwd.hpp"
#include "inotifypp/event.hpp"

namespace inotifypp
{
#define DEFINE_EVENT_TEST(NAME, VALUE) \
inline auto NAME(event_ref    x) -> bool { return x.mask() & VALUE; } \
inline auto NAME(event const& x) -> bool { return x.mask() & VALUE; }

DEFINE_EVENT_TEST(in_access,        IN_ACCESS);
DEFINE_EVENT_TEST(in_attrib,        IN_ATTRIB);
DEFINE_EVENT_TEST(in_close_write,   IN_CLOSE_WRITE);
DEFINE_EVENT_TEST(in_close_nowrite, IN_CLOSE_NOWRITE);
DEFINE_EVENT_TEST(in_create,        IN_CREATE);
DEFINE_EVENT_TEST(in_delete,        IN_DELETE);
DEFINE_EVENT_TEST(in_delete_self,   IN_DELETE_SELF);
DEFINE_EVENT_TEST(in_modify,        IN_MODIFY);
DEFINE_EVENT_TEST(in_move_self,     IN_MOVE_SELF);
DEFINE_EVENT_TEST(in_moved_from,    IN_MOVED_FROM);
DEFINE_EVENT_TEST(in_moved_to,      IN_MOVED_TO);
DEFINE_EVENT_TEST(in_open,          IN_OPEN);

DEFINE_EVENT_TEST(in_move,          IN_MOVE);
DEFINE_EVENT_TEST(in_close,         IN_CLOSE);

DEFINE_EVENT_TEST(in_dont_follow,   IN_DONT_FOLLOW);
DEFINE_EVENT_TEST(in_excl_unlink,   IN_EXCL_UNLINK);
DEFINE_EVENT_TEST(in_mask_add,      IN_MASK_ADD);
DEFINE_EVENT_TEST(in_oneshot,       IN_ONESHOT);
DEFINE_EVENT_TEST(in_onlydir,       IN_ONLYDIR);

DEFINE_EVENT_TEST(in_ignored,       IN_IGNORED);
DEFINE_EVENT_TEST(in_isdir,         IN_ISDIR);
DEFINE_EVENT_TEST(in_q_overflow,    IN_Q_OVERFLOW);
DEFINE_EVENT_TEST(in_unmount,       IN_UNMOUNT);

#undef DEFINE_EVENT_TEST
} // inotifypp

#endif // INOTIFYPP_EVENT_TEST_HPP
