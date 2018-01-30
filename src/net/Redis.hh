/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/11/18.
//

#pragma once

#include "util/Exception.hh"

#include <boost/asio.hpp>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>

#include <string>

namespace hrb {
namespace redis {

// Error enum
enum class Error
{
	ok = REDIS_OK,
	io = REDIS_ERR_IO,
	eof = REDIS_ERR_EOF,
	protocol = REDIS_ERR_PROTOCOL,
	oom = REDIS_ERR_OOM,
	other = REDIS_ERR_OTHER,

	// other logical errors
	command_error = 1000,
	field_not_found
};

std::error_code make_error_code(Error err);
const std::error_category& redis_error_category();

class Reply
{
public:
	explicit Reply(redisReply *r = nullptr) noexcept;
	Reply(const Reply&) = default;
	Reply(Reply&&) = default;
	~Reply() = default;

	Reply& operator=(const Reply&) = default;
	Reply& operator=(Reply&&) = default;

	bool is_string() const {return m_reply->type == REDIS_REPLY_STRING;}

	std::string_view as_string() const noexcept;
	std::string_view as_status() const noexcept;
	std::string_view as_error() const noexcept;
	std::string_view as_any_string() const noexcept;

	explicit operator bool() const noexcept ;

	long as_int() const noexcept;
	long to_int() const noexcept;

	Reply as_array(std::size_t i) const noexcept;
	Reply as_array(std::size_t i, std::error_code& ec) const noexcept;
	std::size_t array_size() const noexcept;

	std::unordered_map<std::string_view, Reply> map_array() const;

	template <std::size_t count>
	auto as_tuple(std::error_code& ec) const
	{
		return as_tuple_impl(ec, std::make_index_sequence<count>{});
	}

private:
	template <std::size_t... index>
	auto as_tuple_impl(std::error_code& ec, std::index_sequence<index...>) const
	{
		return std::make_tuple(as_array(index, ec)...);
	}

private:
	const ::redisReply *m_reply;
};

// Copied from: https://github.com/ryangraham/hiredis-boostasio-adapter/blob/master/boostasio.cpp
class Connection
{
public:
	explicit Connection(
		boost::asio::io_context& bic,
		const std::string& host = "localhost",
		unsigned short port = 6379
	);

	~Connection();

	template <typename Callback, typename... Args>
	void command(Callback&& callback, const char *fmt, Args... args)
	{
		using CallbackType = std::remove_reference_t<Callback>;

		auto callback_ptr = std::make_unique<CallbackType>(std::forward<Callback>(callback));
		auto r = m_ctx ? ::redisAsyncCommand(
			m_ctx, [](redisAsyncContext *ctx, void *pv_reply, void *pv_callback)
			{
				std::error_code ec{static_cast<Error>(ctx->err)};

				Reply reply{static_cast<redisReply *>(pv_reply)};
				if (!reply && !ec)
					ec = Error::command_error;

				std::unique_ptr<CallbackType> callback{static_cast<CallbackType *>(pv_callback)};
				(*callback)(reply, std::move(ec));
			}, callback_ptr.get(), fmt, args...
		) : REDIS_ERR;

		if (r == REDIS_ERR)
			callback(Reply{}, std::error_code{m_ctx ? static_cast<Error>(m_ctx->err) : m_conn_error});
		else
			callback_ptr.release();
	}

	void disconnect();

	boost::asio::io_context& get_io_context() { return m_ioc; }

private:
	static redisAsyncContext *connect(const std::string& host, unsigned short port);

	void run();
	void on_connect_error(const redisAsyncContext *ctx);

private:
	boost::asio::io_context& m_ioc;
	boost::asio::ip::tcp::socket m_socket;

	boost::asio::strand<boost::asio::io_context::executor_type> m_strand;

	redisAsyncContext *m_ctx{};

	bool m_reading{false}, m_writing{false};
	bool m_request_read{false}, m_request_write{false};

	Error   m_conn_error{Error::ok};
	int     m_errno{0};
};

}} // end of namespace

namespace std
{
	template <> struct is_error_code_enum<hrb::redis::Error> : true_type {};
}
