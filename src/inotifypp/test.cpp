#include "inotifypp/inotify.hpp"
#include <cstddef>
#include <cstring>
#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

static
auto data_buffer() -> std::pair<std::vector<std::byte>, inotifypp::event_buffer>
{
	auto data = std::vector<std::byte>(inotifypp::min_buffer_size * 64);
	auto buffer = inotifypp::event_buffer(data);
	return { std::move(data), std::move(buffer) };
}

static
auto data_instance() -> std::pair<std::vector<std::byte>, inotifypp::instance>
{
	auto data = std::vector<std::byte>(inotifypp::min_buffer_size * 64);
	auto instance = inotifypp::instance(data);
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
	inotify_event const* p = buffer.next();
	REQUIRE(p);
	REQUIRE_EQ(p->wd, ev.wd());
	REQUIRE_EQ(p->mask, ev.mask());
	REQUIRE_EQ(p->cookie, ev.cookie());
	REQUIRE_EQ(p->len, ev.len());
	REQUIRE_EQ(std::string(p->name), std::string(ev.name()));
}

TEST_CASE("Watching a directory")
{
	auto [data, instance] = data_instance();
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



/*
int main(int argc, char** argv)
{
	std::vector<std::byte> buffer(inotify::min_buffer_size * 64);
	auto in = inotify::instance(buffer);

	// inotify::watch_item
	auto w = in.add_watch(".", IN_MODIFY | IN_CREATE | IN_DELETE);

	// ...
	do_stuff();
	// ...

	auto ec = std::error_code();

	// inotify::event_ref
	for (;;)
	{
		auto event = in.watch(ec);
		if (ec)
		{
			std::cerr << ec.message() << ':';
			std::cerr << ec.value() << '\n';
			return EXIT_FAILURE;
		}

		const char* kind = "";
		const char* type = "";
		kind = in_isdir(event) ? "directory" : "file";

		type =
			in_create(event) ? "created" :
			in_delete(event) ? "deleted" :
			in_modify(event) ? "modified" : "mishandled";

		std::printf("The %s %s was %s\n", kind, event.name(), type);

	}
	return EXIT_SUCCESS;
}


void do_stuff()
{
	const char* name = tempnam(".", "rand-");
	if (! name) return;
	FILE* file = fopen(name, "w");
	if (! file) return;
	fwrite("foo\nbar\nbaz\n", 12, 1, file);
	fflush(file);
	unlink(name);
}
*/
