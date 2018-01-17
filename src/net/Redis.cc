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

#include <boost/exception/info.hpp>
#include <boost/exception/errinfo_api_function.hpp>
#include <boost/system/error_code.hpp>

#include <cassert>
#include <iostream>

namespace hrb {
namespace redis {

Database::Database(boost::asio::io_context& bic, const std::string& host, unsigned short port) :
	m_ioc{bic},
	m_socket{m_ioc},
	m_ctx{connect(host, port)}
{
	assert(m_ctx);

	m_socket.assign(boost::asio::ip::tcp::v4(), m_ctx->c.fd);

	m_ctx->ev.data = this;
	m_ctx->ev.addRead = [](void *pvthis)
	{
		assert(pvthis);
		auto pthis = static_cast<Database *>(pvthis);
		pthis->m_request_read = true;
		pthis->run();
	};
	m_ctx->ev.delRead = [](void *pvthis)
	{
		static_cast<Database *>(pvthis)->m_request_read = false;
	};
	m_ctx->ev.addWrite = [](void *pvthis)
	{
		assert(pvthis);
		auto pthis = static_cast<Database *>(pvthis);
		pthis->m_request_write = true;
		pthis->run();
	};
	m_ctx->ev.delWrite = [](void *pvthis)
	{
		static_cast<Database *>(pvthis)->m_request_write = false;
	};
	m_ctx->ev.cleanup = [](void *pvthis)
	{
		auto pthis = static_cast<Database *>(pvthis);
		if (pthis->m_socket.is_open())
			pthis->m_socket.release();
	};
	::redisAsyncSetConnectCallback(
		m_ctx, [](const redisAsyncContext *ctx, int status)
		{
			if (status == REDIS_ERR)
				static_cast<Database *>(ctx->ev.data)->on_connect_error(ctx);
		}
	);
	::redisAsyncSetDisconnectCallback(
		m_ctx, [](const redisAsyncContext *ctx, int status)
		{
			// The caller will free the context anyway, so set our own context to nullptr
			// to avoid double free
			static_cast<Database *>(ctx->ev.data)->m_ctx = nullptr;
		}
	);
}

void Database::on_connect_error(const redisAsyncContext *ctx)
{
	std::cout << "connect error: " << ctx->err << " " << ctx->c.err << " " << errno << " " << " \"" << ctx->errstr << "\"" << std::endl;
	m_conn_error = static_cast<Error>(ctx->c.err);
	if (m_conn_error == Error::io)
		m_errno = errno;

	m_ctx = nullptr;
}


Database::~Database()
{
	if (m_ctx)
		::redisAsyncDisconnect(m_ctx);
}

void Database::run()
{
	if (m_request_read && !m_reading && m_ctx)
	{
		m_reading = true;
		m_socket.async_wait(
			boost::asio::socket_base::wait_read, [this](auto&& ec)
			{
				m_reading = false;
				if (!ec && m_ctx)
					::redisAsyncHandleRead(m_ctx);
				if (!ec || ec == boost::system::errc::operation_would_block)
					run();
			}
		);
	}
	if (m_request_write && !m_writing && m_ctx)
	{
		m_writing = true;
		m_socket.async_wait(
			boost::asio::socket_base::wait_write, [this](auto&& ec)
			{
				m_writing = false;
				if (!ec && m_ctx)
					::redisAsyncHandleWrite(m_ctx);
				if (!ec || ec == boost::system::errc::operation_would_block)
					run();
			}
		);
	}
}

redisAsyncContext *Database::connect(const std::string& host, unsigned short port)
{
	auto ctx = ::redisAsyncConnect(host.c_str(), port);
	if (ctx->err)
	{
		std::cout << "connect error: " << ctx->errstr << std::endl;
		BOOST_THROW_EXCEPTION(std::runtime_error("cannot connect"));
	}
	return ctx;
}

void Database::disconnect()
{
	::redisAsyncDisconnect(m_ctx);
	m_ctx = nullptr;
}

Reply::Reply(redisReply *r) :
	m_reply{r}
{
	static const redisReply empty{};
	if (!m_reply)
		m_reply = &empty;
}

std::string_view Reply::as_string() const
{
	return (m_reply->type == REDIS_REPLY_STRING || m_reply->type == REDIS_REPLY_STATUS) ?
		std::string_view{m_reply->str, static_cast<std::size_t>(m_reply->len)} : std::string_view{};
}

Reply Reply::as_array(std::size_t i) const
{
	return m_reply->type == REDIS_REPLY_ARRAY && i < m_reply->elements ?
		Reply{m_reply->element[i]} : Reply{};
}

std::size_t Reply::array_size() const
{
	return m_reply->type == REDIS_REPLY_ARRAY ? m_reply->elements : 0ULL;
}

long Reply::as_int() const
{
	return m_reply->type == REDIS_REPLY_INTEGER ? m_reply->integer : 0;
}

struct ErrorCategory : std::error_category
{
	const char *name() const noexcept override;
	std::string message(int ev) const override;
};
const ErrorCategory redis_error{};

const char *ErrorCategory::name() const noexcept
{
	return "redis";
}

std::string ErrorCategory::message(int ev) const
{
	switch (ev)
	{
		case REDIS_OK: return "no error";
		case REDIS_ERR_IO: return "IO error";
		case REDIS_ERR_EOF: return "EOF error";
		case REDIS_ERR_PROTOCOL: return "protocol error";
		case REDIS_ERR_OOM: return "out-of-memory error";
		case REDIS_ERR_OTHER: return "other error";
		default: return "unknown error";
	}
}

std::error_code make_error_code(Error err)
{
	return std::error_code(static_cast<int>(err), redis_error);
}

}} // end of namespace
