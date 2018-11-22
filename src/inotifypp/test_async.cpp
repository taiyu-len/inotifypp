#include "inotifypp/async_watch.hpp"
#include "inotifypp/inotify.hpp"
#include <boost/asio/io_context.hpp>
#include <boost/asio/use_future.hpp>
#include <cstddef>
#include <cstring>
#include <doctest/doctest.h>
#include <string>
#include <system_error>
#include <vector>
#include <future>

static
auto make_instance(boost::asio::io_context& io)
-> std::pair<std::vector<std::byte>, inotifypp::instance>
{
	auto data = std::vector<std::byte>(inotifypp::min_buffer_size * 64);
	auto instance = inotifypp::instance(io, data);
	return { std::move(data), std::move(instance) };
}

static
inotifypp::mask_t events[] = {
	// File
	IN_CREATE, IN_OPEN,
	IN_MODIFY, IN_CLOSE_WRITE,
	IN_DELETE,
	// Directory
	IN_CREATE|IN_ISDIR, IN_DELETE|IN_ISDIR,
	// delete self
	IN_DELETE_SELF, IN_IGNORED,
};

static
const size_t events_n = sizeof(events) / sizeof(*events);

struct event_handler
{
	inotifypp::instance* in;
	size_t i = 0;

	void initiate()
	{
		async_watch(*in, std::move(*this));
	}
	void operator()(std::error_code const& ec, inotifypp::event_ref er)
	{
		if (ec) throw std::system_error(ec);
		REQUIRE_EQ(er.mask(), events[i++]);
		// continue calling if we still have events
		if (! in_ignored(er))
		{
			initiate();
		}
	}
};

TEST_CASE("Async watch")
{
	auto io = boost::asio::io_context{};
	auto [data, in] = make_instance(io);
	auto file = std::string("file.txt");
	bool all_items_inserted = true;
	for (auto event : events)
	{
		inotifypp::event ev({1, event, 0, 16}, file);
		all_items_inserted = all_items_inserted && in.buffer().try_push(ev);
	}
	REQUIRE(all_items_inserted);
	event_handler{&in}.initiate();
	io.run();
}

TEST_CASE("Use Async Futures")
{
	auto io = boost::asio::io_context{};
	auto [data, in] = make_instance(io);
	bool all_items_inserted = true;
	for (auto event : events)
	{
		inotifypp::event ev({1, event, 0, 16}, "filename");
		all_items_inserted = all_items_inserted && in.buffer().try_push(ev);
	}
	REQUIRE(all_items_inserted);

	// since we are using std::error_code, instead of boost::system::error_code,
	// we dont get a nice std::future<event_ref>, but a std::future<tuple<...>> instead
	auto fref = async_watch(in, boost::asio::use_future);
	io.run_one();
	REQUIRE(fref.valid());
	REQUIRE(std::get<1>(fref.get()).mask() == events[0]);
}
