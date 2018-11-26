#ifndef INOTIFYPP_ASYNC_WATCH_HPP
#define INOTIFYPP_ASYNC_WATCH_HPP
#include "inotifypp/event.hpp"
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
template<typename Handler>
struct async_watch_op
{
	instance& _in;
	Handler   _h;

	void operator()(boost::system::error_code bec, int bytes_read)
	{
		auto ec = std::make_error_code(std::errc(bec.value()));
		if (ec) _h(ec, event_ref{});
		else {
			_in.buffer().grow(bytes_read);
			(*this)();
		}
	}
	void operator()()
	{
		_h(std::error_code{}, event_ref(_in.buffer().pop()));
	}

	using executor_type = typename boost::asio::associated_executor<
		Handler, instance::executor_type>::type;

	auto get_executor() const noexcept -> executor_type {
		return boost::asio::get_associated_executor(_h, _in.get_executor());
	}

};


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

	auto op = async_watch_op<handler_type>{in, std::move(h)};

	// no more events
	if (in.buffer().remaining() == 0)
	{
		in.buffer().clear();
		auto b = boost::asio::buffer(in.buffer().data(), in.buffer().size());
		in.sd().async_read_some(b, std::move(op));
	}
	else
	{
		auto ex = op.get_executor();
		boost::asio::post(ex, std::move(op));
	}
	return r.get();
}

} // inotifypp

#endif // INOTIFYPP_ASYNC_WATCH_HPP
