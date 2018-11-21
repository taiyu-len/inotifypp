#include "inotifypp/inotify.hpp"
#include <boost/asio/io_context.hpp>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <sys/stat.h>
#include <tuple>
#include <vector>

static
auto data_buffer() -> std::pair<std::vector<std::byte>, inotifypp::event_buffer>
{
	auto data = std::vector<std::byte>(inotifypp::min_buffer_size * 64);
	auto buffer = inotifypp::event_buffer(data);
	return { std::move(data), std::move(buffer) };
}

static
auto make_instance(boost::asio::io_context& io) -> std::pair<std::vector<std::byte>, inotifypp::instance>
{
	auto data = std::vector<std::byte>(inotifypp::min_buffer_size * 64);
	auto instance = inotifypp::instance(io, data);
	return { std::move(data), std::move(instance) };
}

static
auto tmp_dir() -> std::filesystem::path
{
	auto dir = std::filesystem::temp_directory_path();
	char buffer[TMP_MAX];
	return dir / std::tmpnam(buffer);
}

TEST_CASE("Manually creating events")
{
	auto [data, buffer] = data_buffer();
	auto file = std::string{"muh filename.txt"};
	inotifypp::event ev({1, IN_CREATE, 0, 32}, file);
	REQUIRE(buffer.try_push(ev));
	inotify_event const* p = buffer.pop();
	REQUIRE(p);
	REQUIRE_EQ(p->wd, ev.wd());
	REQUIRE_EQ(p->mask, ev.mask());
	REQUIRE_EQ(p->cookie, ev.cookie());
	REQUIRE_EQ(p->len, ev.len());
	REQUIRE_EQ(std::string(p->name), std::string(ev.name()));
}

TEST_CASE("Watching a directory")
{
	auto io = boost::asio::io_context{};
	auto [data, instance] = make_instance(io);
	auto dir = tmp_dir();
	auto filename = dir/"muh file.txt";

	std::filesystem::create_directory(dir);
	instance.add_watch(dir.c_str(), IN_MODIFY | IN_CREATE | IN_DELETE | IN_DELETE_SELF).forget();
	// Create File
	auto file = std::fstream(filename, std::ios::out);
	file.close();
	REQUIRE(in_create(instance.watch()));

	// Modify file
	file.open(filename);
	file << "foo" << std::endl;
	file.close();
	REQUIRE(in_modify(instance.watch()));

	// Remove file
	std::filesystem::remove(filename);
	REQUIRE(in_delete(instance.watch()));

	// remove watched directory
	std::filesystem::remove(dir);
	REQUIRE(in_delete_self(instance.watch()));
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

struct event_handler {
	inotifypp::instance* in;
	size_t i = 0;

	event_handler(inotifypp::instance &in)
	: in(&in) {}

	void initiate() {
		in->async_watch(std::move(*this));
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
	auto h  = event_handler(in);
	auto file = std::string("file.txt");
	for (auto event : events)
	{
		inotifypp::event ev({1, event, 0, 16}, file);
		REQUIRE(in.buffer().try_push(ev));
	}
	h.initiate();
	io.run();
}
