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
void handler(std::error_code const& ec, inotifypp::event_ref er)
{
	if (ec) throw ec;
	if (in_ignored(er))
	{
		MESSAGE("Watch #" << er.wd() << " Has been removed \n");
		return;
	}
	auto type = in_isdir(er) ? "Directory" : "File";
	auto kind =
		in_create(er) ? "was created" :
		in_modify(er) ? "was modified" :
		in_attrib(er) ? "had attributes changed" :
		in_close(er)  ? "was closed" :
		in_move(er)   ? "was moved" :
		in_delete(er) ? "was deleted" :
		in_open(er)   ? "was opened" : "did something else";
	MESSAGE(type << " " << er.name() << " " << kind << '\n');
}

TEST_CASE("Async watch")
{
	auto io = boost::asio::io_context{};
	auto [data, instance] = make_instance(io);
	auto dir = tmp_dir();
	std::filesystem::create_directory(dir);
	instance.add_watch(dir.c_str(), IN_ALL_EVENTS).forget();
	instance.async_watch(handler);

	namespace fs = std::filesystem;
	fs::path files[] = {
		dir/"a.txt",
		dir/"b.txt",
		dir/"c.txt",
		dir/"subdir",
	};
	{
		auto f1 = std::fstream(files[0], std::ios::out);
		auto f2 = std::fstream(files[1], std::ios::out);
		f1.close(); f2.close();
		f1.open(files[0]); f2.open(files[1]);
		f1 << "foo\nbar\n"; f2 << "bar\nbaz\n";
		f1.close(); f2.close();
	}
	fs::rename(files[0], files[2]);
	fs::create_directory(files[3]);
	fs::remove(files[1]);
	fs::remove(files[2]);
	fs::remove(files[3]);
	fs::remove(dir);
	io.run_for(std::chrono::seconds(1));
}
