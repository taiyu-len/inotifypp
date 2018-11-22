#ifndef INOTIFYPP_ASYNC_WATCH_HPP
#define INOTIFYPP_ASYNC_WATCH_HPP
#include "inotifypp/fwd.hpp"
#include "inotifypp/instance.hpp"
#include <boost/asio/async_result.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/post.hpp>
#include <boost/system/error_code.hpp>
#include <system_error>
#include <type_traits>

namespace inotifypp
{

template<typename Token>
// Token is used to construct a handler, which calls void(std::error_code, event_ref)
// will spawn an async job in sd's executor that calls h once
decltype(auto) async_watch(instance& in, Token && token)
{
	using result_type = boost::asio::async_result<
		std::decay_t<Token>,
		void(std::error_code, event_ref)>;
	using handler_type = typename result_type::completion_handler_type;
	handler_type h(std::forward<Token>(token));
	result_type r(h);

	auto fn = [&in, h = std::move(h)] (
		boost::system::error_code const& ec,
		size_t bytes_read) mutable
	{
		if (ec)
		{
			h(std::make_error_code(std::errc(ec.value())), event_ref{});
		}
		else
		{
			in.buffer().grow(bytes_read);
			h(std::error_code{}, event_ref(in.buffer().pop()));
		}
	};
	// no more events
	if (in.buffer().remaining() == 0)
	{
		in.buffer().clear();
		auto b = boost::asio::buffer(in.buffer().data(), in.buffer().size());
		in.sd().async_read_some(b, std::move(fn));
	}
	// existing events
	else
	{
		auto f = [fn=std::move(fn)] () mutable
		{
			fn({}, 0);
		};
		boost::asio::post(in.sd().get_executor(), std::move(f));
	}
	return r.get();
}

} // inotifypp

#endif // INOTIFYPP_ASYNC_WATCH_HPP
