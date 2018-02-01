/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/11/18.
//


#include "Redis.hh"

#include "util/Backtrace.hh"
#include "util/Error.hh"

#include <boost/exception/info.hpp>
#include <boost/exception/errinfo_api_function.hpp>
#include <boost/system/error_code.hpp>

#include <cassert>
#include <iostream>

namespace hrb {
namespace redis {

Connection::Connection(boost::asio::io_context& bic, const std::string& host, unsigned short port) :
	m_ioc{bic},
	m_socket{m_ioc},
	m_strand{m_socket.get_executor()},
	m_ctx{connect(host, port)}
{
	assert(m_ctx);

	m_socket.assign(boost::asio::ip::tcp::v4(), m_ctx->c.fd);

	m_ctx->ev.data = this;
	m_ctx->ev.addRead = [](void *pvthis)
	{
		assert(pvthis);
		auto pthis = static_cast<Connection *>(pvthis);
		pthis->m_request_read = true;
		pthis->run();
	};
	m_ctx->ev.delRead = [](void *pvthis)
	{
		static_cast<Connection *>(pvthis)->m_request_read = false;
	};
	m_ctx->ev.addWrite = [](void *pvthis)
	{
		assert(pvthis);
		auto pthis = static_cast<Connection *>(pvthis);
		pthis->m_request_write = true;
		pthis->run();
	};
	m_ctx->ev.delWrite = [](void *pvthis)
	{
		static_cast<Connection *>(pvthis)->m_request_write = false;
	};
	m_ctx->ev.cleanup = [](void *pvthis)
	{
		auto pthis = static_cast<Connection *>(pvthis);
		if (pthis->m_socket.is_open())
			pthis->m_socket.release();
	};
	::redisAsyncSetConnectCallback(
		m_ctx, [](const redisAsyncContext *ctx, int status)
		{
			if (status == REDIS_ERR)
				static_cast<Connection *>(ctx->ev.data)->on_connect_error(ctx);
		}
	);
	::redisAsyncSetDisconnectCallback(
		m_ctx, [](const redisAsyncContext *ctx, int status)
		{
			// The caller will free the context anyway, so set our own context to nullptr
			// to avoid double free
			static_cast<Connection *>(ctx->ev.data)->m_ctx = nullptr;
		}
	);
}

void Connection::on_connect_error(const redisAsyncContext *ctx)
{
	// save the error enum and errno so that the next command() will return it
	m_conn_error = static_cast<Error>(ctx->c.err);
	if (m_conn_error == Error::io)
		m_errno = errno;

	m_ctx = nullptr;
}


Connection::~Connection()
{
	if (m_ctx)
		::redisAsyncDisconnect(m_ctx);
}

void Connection::run()
{
	if (m_request_read && !m_reading && m_ctx)
	{
		m_reading = true;
		m_socket.async_wait(
			boost::asio::socket_base::wait_read, boost::asio::bind_executor(m_strand, [this](auto&& ec)
			{
				m_reading = false;
				if (!ec && m_ctx)
					::redisAsyncHandleRead(m_ctx);
				if (!ec || ec == boost::system::errc::operation_would_block)
					run();
			})
		);
	}
	if (m_request_write && !m_writing && m_ctx)
	{
		m_writing = true;
		m_socket.async_wait(
			boost::asio::socket_base::wait_write, boost::asio::bind_executor(m_strand, [this](auto&& ec)
			{
				m_writing = false;
				if (!ec && m_ctx)
					::redisAsyncHandleWrite(m_ctx);
				if (!ec || ec == boost::system::errc::operation_would_block)
					run();
			})
		);
	}
}

redisAsyncContext *Connection::connect(const std::string& host, unsigned short port)
{
	auto ctx = ::redisAsyncConnect(host.c_str(), port);
	if (ctx->err)
	{
		std::cout << "connect error: " << ctx->errstr << std::endl;
		BOOST_THROW_EXCEPTION(std::runtime_error("cannot connect"));
	}
	return ctx;
}

void Connection::disconnect()
{
	if (m_ctx)
		::redisAsyncDisconnect(m_ctx);
	m_ctx = nullptr;
}

Reply::Reply(const redisReply *r) noexcept :
	m_reply{r}
{
	static const redisReply empty{};
	if (!m_reply)
		m_reply = &empty;
}

std::string_view Reply::as_string() const noexcept
{
	return (m_reply->type == REDIS_REPLY_STRING) ? as_any_string() : std::string_view{};
}

std::string_view Reply::as_status() const noexcept
{
	return (m_reply->type == REDIS_REPLY_STATUS) ? as_any_string() : std::string_view{};
}

std::string_view Reply::as_error() const noexcept
{
	return (m_reply->type == REDIS_REPLY_ERROR) ? as_any_string() : std::string_view{};
}

std::string_view Reply::as_any_string() const noexcept
{
	return (m_reply->type == REDIS_REPLY_STRING || m_reply->type == REDIS_REPLY_STATUS || m_reply->type == REDIS_REPLY_ERROR) ?
		std::string_view{m_reply->str, static_cast<std::size_t>(m_reply->len)} : std::string_view{};
}

Reply Reply::as_array(std::size_t i) const noexcept
{
	return m_reply->type == REDIS_REPLY_ARRAY && i < m_reply->elements ?
		Reply{m_reply->element[i]} : Reply{};
}

Reply Reply::as_array(std::size_t i, std::error_code& ec) const noexcept
{
	// If there is already an error, do nothing and because we can't report error.
	if (ec)
		return Reply{};

	if (m_reply->type == REDIS_REPLY_ARRAY && i < m_reply->elements)
	{
		return Reply{m_reply->element[i]};
	}
	else
	{
		ec = Error::field_not_found;
		return Reply{};
	}
}

Reply::iterator Reply::begin() const
{
	return iterator{&m_reply->element[0]};
}

Reply::iterator Reply::end() const
{
	return iterator{&m_reply->element[m_reply->elements]};
}

Reply Reply::operator[](std::size_t i) const noexcept
{
	return as_array(i);
}

std::size_t Reply::array_size() const noexcept
{
	return m_reply->type == REDIS_REPLY_ARRAY ? m_reply->elements : 0ULL;
}

long Reply::as_int() const noexcept
{
	return m_reply->type == REDIS_REPLY_INTEGER ? m_reply->integer : 0;
}

Reply::operator bool() const noexcept
{
	return m_reply->type != REDIS_REPLY_ERROR;
}

long Reply::to_int() const noexcept
{
	auto s = as_any_string();
	return s.empty() ? 0 : std::stol(std::string{s});
	// auto r = std::from_chars()
}

const std::error_category& redis_error_category()
{
	struct Cat : std::error_category
	{
		const char *name() const noexcept override { return "redis"; }

		std::string message(int ev) const override
		{
			switch (static_cast<Error>(ev))
			{
				case Error::ok: return "success";
				case Error::io: return "IO error";
				case Error::eof: return "EOF error";
				case Error::protocol: return "protocol error";
				case Error::oom: return "out-of-memory error";
				case Error::other: return "other error";
				case Error::command_error: return "command error";
				case Error::field_not_found: return "field not found";
				default: return "unknown error";
			}
		}
	};
	static const Cat cat{};
	return cat;
}

std::error_code make_error_code(Error err)
{
	return std::error_code(static_cast<int>(err), redis_error_category());
}

}} // end of namespace
