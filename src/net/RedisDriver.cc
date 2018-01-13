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

#include <cassert>
#include <iostream>

namespace hrb {

RedisDriver::RedisDriver(boost::asio::io_context& bic, const std::string& host, unsigned short port) :
	m_bic{bic},
	m_read{m_bic},
	m_write{m_bic},
	m_ctx{connect(host, port)}
{
	assert(m_ctx);

	m_ctx->ev.data    = this;
	m_ctx->ev.addRead = [](void *pvthis)
	{
		assert(pvthis);
		auto pthis = static_cast<RedisDriver*>(pvthis);
		if (!pthis->m_read.is_open())
			pthis->do_read();
	};
	m_ctx->ev.delRead = RedisDriver::del_read;
	m_ctx->ev.addWrite = [](void *pvthis)
	{
		assert(pvthis);
		auto pthis = static_cast<RedisDriver*>(pvthis);
		if (!pthis->m_write.is_open())
			pthis->do_write();
	};
	m_ctx->ev.delWrite = RedisDriver::del_write;
	m_ctx->ev.cleanup = [](void *pvthis)
	{
		RedisDriver::del_read(pvthis);
		RedisDriver::del_write(pvthis);
	};
	::redisAsyncSetConnectCallback(m_ctx, [](const redisAsyncContext *ctx, int status)
	{
		if (ctx->err)
			BOOST_THROW_EXCEPTION(Error() << ErrorMsg(ctx->errstr));
	});
	::redisAsyncSetDisconnectCallback(m_ctx, [](const redisAsyncContext *ctx, int status)
	{
		std::cout << "disconnected context " << ctx << " " << status << std::endl;

		if (ctx->err)
			BOOST_THROW_EXCEPTION(Error() << ErrorMsg(ctx->errstr));

		auto pthis = static_cast<RedisDriver*>(ctx->ev.data);
		pthis->m_ctx = nullptr;
	});
}

RedisDriver::~RedisDriver()
{
//	if (m_ctx)
//		::redisAsyncDisconnect(m_ctx);
}

void RedisDriver::do_read()
{
	m_read.assign(boost::asio::ip::tcp::v4(), m_ctx->c.fd);
	m_read.async_wait(boost::asio::socket_base::wait_read, [this](auto&& ec)
	{
		// release socket before calling redisAsyncHandleRead() because it may close the socket
		if (!ec)
			m_read.release();

		if (m_ctx)
			::redisAsyncHandleRead(m_ctx);
	});
}

void RedisDriver::do_write()
{
	m_write.assign(boost::asio::ip::tcp::v4(), m_ctx->c.fd);
	m_write.async_wait(boost::asio::socket_base::wait_write, [this](auto&& ec)
	{
		if (!ec)
			m_write.release();

		if (m_ctx)
			::redisAsyncHandleWrite(m_ctx);
	});
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
}

void RedisDriver::del_read(void *pvthis)
{
	assert(pvthis);
	auto pthis = static_cast<RedisDriver*>(pvthis);
	if (pthis->m_read.is_open())
	{
		boost::system::error_code ec;
		pthis->m_read.cancel(ec);
		pthis->m_read.release();
	}
}

void RedisDriver::del_write(void *pvthis)
{
	assert(pvthis);
	auto pthis = static_cast<RedisDriver*>(pvthis);
	if (pthis->m_write.is_open())
	{
		boost::system::error_code ec;
		pthis->m_write.cancel(ec);
		pthis->m_write.release();
	}
}

} // end of namespace
