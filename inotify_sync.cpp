#include "inotify.hpp"
#include <iostream>
#include <random>
#include <string>
#include <vector>

static void do_stuff();

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
	for (int i = 0; i < 3; ++i)
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
