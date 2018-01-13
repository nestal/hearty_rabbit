/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/11/18.
//

#include <hiredis/async.h>
#include "RedisDriver.hh"

#include <cassert>
#include <iostream>

#include <boost/core/demangle.hpp>

#define UNW_LOCAL_ONLY
#include <libunwind.h>

namespace hrb {

// Call this function to get a backtrace.
void backtrace()
{
	unw_cursor_t cursor;
	unw_context_t context;

	// Initialize cursor to current frame for local unwinding.
	unw_getcontext(&context);
	unw_init_local(&cursor, &context);

	// Unwind frames one by one, going up the frame stack.
	while (unw_step(&cursor) > 0)
	{
		unw_word_t offset, pc;
		unw_get_reg(&cursor, UNW_REG_IP, &pc);
		if (pc == 0)
		{
			break;
		}
		printf("0x%lx:", pc);

		char sym[1024];
		if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0)
		{
			auto demangled_sym = boost::core::demangle(sym);
			printf(" (%s+0x%lx)\n", demangled_sym.c_str(), offset);
		}
		else
		{
			printf(" -- error: unable to obtain symbol name for this frame\n");
		}
	}
}

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
	m_ctx->ev.delRead = [](void *pvthis)
	{
		assert(pvthis);
		auto pthis = static_cast<RedisDriver*>(pvthis);
		if (pthis->m_read.is_open())
		{
			boost::system::error_code ec;
			pthis->m_read.cancel(ec);
			pthis->m_read.release();
		}
	};
	m_ctx->ev.addWrite = [](void *pvthis)
	{
		assert(pvthis);
		auto pthis = static_cast<RedisDriver*>(pvthis);
		if (!pthis->m_write.is_open())
			pthis->do_write();
	};
	m_ctx->ev.delWrite = [](void *pvthis)
	{
		assert(pvthis);
		auto pthis = static_cast<RedisDriver*>(pvthis);
		if (pthis->m_write.is_open())
		{
			boost::system::error_code ec;
			pthis->m_write.cancel(ec);
			pthis->m_write.release();
		}
	};
	m_ctx->ev.cleanup = [](void *pvthis)
	{
		assert(pvthis);
		auto pthis = static_cast<RedisDriver*>(pvthis);
		(pthis->m_ctx->ev.delRead)(pvthis);
		(pthis->m_ctx->ev.delWrite)(pvthis);
	};
	::redisAsyncSetConnectCallback(m_ctx, [](const redisAsyncContext *ctx, int status)
	{
		hrb::backtrace();
		abort();
	});
	::redisAsyncSetDisconnectCallback(m_ctx, [](const redisAsyncContext *ctx, int status)
	{
		std::cout << "error???" << ctx->errstr << std::endl;
	});
}

RedisDriver::~RedisDriver()
{
	redisAsyncFree(m_ctx);
}

void RedisDriver::do_read()
{
	m_read.assign(boost::asio::ip::tcp::v4(), m_ctx->c.fd);
	m_read.async_wait(boost::asio::socket_base::wait_read, [this](auto&& ec)
	{
		// release socket before calling redisAsyncHandleRead() because it may close the socket
		m_read.release();
		redisAsyncHandleRead(m_ctx);
	});
}

void RedisDriver::do_write()
{
	m_write.assign(boost::asio::ip::tcp::v4(), m_ctx->c.fd);
	m_write.async_wait(boost::asio::socket_base::wait_write, [this](auto&& ec)
	{
		m_write.release();
		redisAsyncHandleWrite(m_ctx);
	});
}

redisAsyncContext* RedisDriver::connect(const std::string& host, unsigned short port)
{
	auto ctx = redisAsyncConnect(host.c_str(), port);
	std::cout << "connect " << ctx->errstr << std::endl;
	if (ctx->err)
		throw std::runtime_error(ctx->errstr);
	return ctx;
}

} // end of namespace
