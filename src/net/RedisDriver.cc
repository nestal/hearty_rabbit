/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/11/18.
//


#include "RedisDriver.hh"

#include "util/Backtrace.hh"

#include <boost/exception/info.hpp>
#include <boost/exception/errinfo_api_function.hpp>
#include <boost/system/error_code.hpp>

#include <cassert>
#include <iostream>

namespace hrb {

RedisDriver::RedisDriver(boost::asio::io_context& bic, const std::string& host, unsigned short port) :
	m_ioc{bic},
	m_socket{m_ioc},
	m_ctx{connect(host, port)}
{
	assert(m_ctx);

	m_socket.assign(boost::asio::ip::tcp::v4(), m_ctx->c.fd);

	m_ctx->ev.data    = this;
	m_ctx->ev.addRead = [](void *pvthis)
	{
		assert(pvthis);
		auto pthis = static_cast<RedisDriver*>(pvthis);
		pthis->m_request_read = true;
		pthis->run();
	};
	m_ctx->ev.delRead = [](void *pvthis)
	{
		static_cast<RedisDriver*>(pvthis)->m_request_read = false;
	};
	m_ctx->ev.addWrite = [](void *pvthis)
	{
		assert(pvthis);
		auto pthis = static_cast<RedisDriver*>(pvthis);
		pthis->m_request_write = true;
		pthis->run();
	};
	m_ctx->ev.delWrite = [](void *pvthis)
	{
		static_cast<RedisDriver*>(pvthis)->m_request_write = false;
	};
	m_ctx->ev.cleanup = [](void *pvthis)
	{
		auto pthis = static_cast<RedisDriver*>(pvthis);
		pthis->m_socket.release();
	};
	::redisAsyncSetDisconnectCallback(m_ctx, [](const redisAsyncContext *ctx, int status)
	{
		if (ctx->err)
			BOOST_THROW_EXCEPTION(Error() << ErrorMsg(ctx->errstr));

		auto pthis = static_cast<RedisDriver*>(ctx->ev.data);
		pthis->m_ctx = nullptr;
	});
}

RedisDriver::~RedisDriver()
{
	if (m_ctx)
		::redisAsyncDisconnect(m_ctx);
}

void RedisDriver::run()
{
	if (m_request_read && !m_reading && m_ctx)
	{
		m_reading = true;
		m_socket.async_wait(boost::asio::socket_base::wait_read, [this](auto&& ec)
		{
			m_reading = false;
			if (!ec && m_ctx)
				::redisAsyncHandleRead(m_ctx);
			if (!ec || ec == boost::system::errc::operation_would_block)
				run();
		});
	}
	if (m_request_write && !m_writing && m_ctx)
	{
		m_writing = true;
		m_socket.async_wait(boost::asio::socket_base::wait_write, [this](auto&& ec)
		{
			m_writing = false;
			if (!ec && m_ctx)
				::redisAsyncHandleWrite(m_ctx);
			if (!ec || ec == boost::system::errc::operation_would_block)
				run();
		});
	}
}

redisAsyncContext* RedisDriver::connect(const std::string& host, unsigned short port)
{
	auto ctx = ::redisAsyncConnect(host.c_str(), port);
	if (ctx->err)
		BOOST_THROW_EXCEPTION(Error()
			<< ErrorMsg(ctx->errstr)
			<< boost::errinfo_api_function("redisAsyncConnect")
		);
	return ctx;
}

void RedisDriver::disconnect()
{
	::redisAsyncDisconnect(m_ctx);
	m_ctx = nullptr;
}

RedisReply::RedisReply(redisReply *r) :
	m_reply{r}
{
	assert(m_reply);
}

std::string_view RedisReply::as_string() const
{
	return (m_reply->type == REDIS_REPLY_STRING || m_reply->type == REDIS_REPLY_STATUS) ?
		std::string_view{m_reply->str, static_cast<std::size_t>(m_reply->len)} : std::string_view{};
}

RedisReply RedisReply::as_array(std::size_t i) const
{
	return m_reply->type == REDIS_REPLY_ARRAY && i < m_reply->elements ?
		RedisReply{m_reply->element[i]} : RedisReply{};
}

std::size_t RedisReply::array_size() const
{
	return m_reply->type == REDIS_REPLY_ARRAY ? m_reply->elements : 0ULL;
}


} // end of namespace
