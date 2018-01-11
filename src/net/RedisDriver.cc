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

namespace hrb {

RedisDriver::RedisDriver(boost::asio::io_context& bic) :
	m_bic{bic},
	m_read{m_bic},
	m_write{m_bic}
{
	assert(m_ctx);

	m_ctx->ev.data = this;
	m_ctx->ev.addRead = RedisDriver::OnAddRead;
	m_ctx->ev.delRead = RedisDriver::OnDelRead;
	m_ctx->ev.addWrite = RedisDriver::OnAddWrite;
	m_ctx->ev.delWrite = RedisDriver::OnDelWrite;
	m_ctx->ev.cleanup = RedisDriver::OnCleanUp;
}

RedisDriver::~RedisDriver()
{
	redisAsyncFree(m_ctx);
}

void RedisDriver::OnAddRead(void *pvthis)
{
	if (auto pthis = reinterpret_cast<RedisDriver*>(pvthis); !pthis->m_read.is_open())
		pthis->DoRead();
}
void RedisDriver::DoRead()
{
	m_read.assign(boost::asio::ip::tcp::v4(), m_ctx->c.fd);
	m_read.async_wait(boost::asio::socket_base::wait_read, [this](auto&& ec)
	{
		// release socket before calling redisAsyncHandleRead() because it may close the socket
		m_read.release();
		redisAsyncHandleRead(m_ctx);
	});
}

void RedisDriver::OnDelRead(void *pvthis)
{
	if (auto pthis = reinterpret_cast<RedisDriver*>(pvthis); pthis->m_read.is_open())
	{
		boost::system::error_code ec;
		pthis->m_read.cancel(ec);
		pthis->m_read.release();
	}
}

void RedisDriver::OnAddWrite(void *pvthis)
{
	if (auto pthis = reinterpret_cast<RedisDriver*>(pvthis); !pthis->m_write.is_open())
		pthis->DoWrite();
}
void RedisDriver::DoWrite()
{
	m_write.assign(boost::asio::ip::tcp::v4(), m_ctx->c.fd);
	m_write.async_wait(boost::asio::socket_base::wait_write, [this](auto&& ec)
	{
		m_write.release();
		redisAsyncHandleWrite(m_ctx);
	});
}

void RedisDriver::OnDelWrite(void *pvthis)
{
	if (auto pthis = reinterpret_cast<RedisDriver*>(pvthis); pthis->m_write.is_open())
	{
		boost::system::error_code ec;
		pthis->m_write.cancel(ec);
		pthis->m_write.release();
	}
}

void RedisDriver::OnCleanUp(void *pvthis)
{
	OnDelRead(pvthis);
	OnDelWrite(pvthis);
}



}