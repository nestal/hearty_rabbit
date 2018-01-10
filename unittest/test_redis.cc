/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/10/18.
//

#include <boost/asio.hpp>

#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <iostream>
#include <cassert>

class RedisDriver
{
public:
	RedisDriver(boost::asio::io_context& bic) :
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
	~RedisDriver()
	{
		redisAsyncFree(m_ctx);
	}

private:
	static void OnAddRead(void *pvthis)
	{
		auto pthis = reinterpret_cast<RedisDriver*>(pvthis);
		std::cout << "OnAddRead()" << pthis->m_read.native_handle() << " " << pthis->m_ctx->c.fd << std::endl;

		if (!pthis->m_read.is_open())
		{
			pthis->m_read.assign(boost::asio::ip::tcp::v4(), pthis->m_ctx->c.fd);
			pthis->DoRead();
		}
	}
	void DoRead()
	{
		m_read.async_wait(boost::asio::socket_base::wait_read, [this](auto&& ec)
		{
			OnRead(ec);
		});
	}
	void OnRead(const boost::system::error_code& ec)
	{
		m_read.release();
		redisAsyncHandleRead(m_ctx);

//		if (!ec)
//			DoRead();
	}
	static void OnDelRead(void *pvthis)
	{
		std::cout << "OnDelRead()" << std::endl;

		auto pthis = reinterpret_cast<RedisDriver*>(pvthis);

		if (pthis->m_read.is_open())
		{
			boost::system::error_code ec;
			pthis->m_read.cancel(ec);
			pthis->m_read.release();
		}
	}
	static void OnAddWrite(void *pvthis)
	{
		auto pthis = reinterpret_cast<RedisDriver*>(pvthis);

		std::cout << "OnAddWrite()" << pthis->m_write.native_handle() << " " << pthis->m_ctx->c.fd << std::endl;
		if (!pthis->m_write.is_open())
		{
			pthis->m_write.assign(boost::asio::ip::tcp::v4(), pthis->m_ctx->c.fd);
			pthis->DoWrite();
		}
	}
	void DoWrite()
	{
		m_write.async_wait(boost::asio::socket_base::wait_write, [this](auto&& ec)
		{
			OnWrite(ec);
		});
	}
	void OnWrite(const boost::system::error_code& ec)
	{
		m_write.release();
		redisAsyncHandleWrite(m_ctx);

//		if (!ec)
//			DoWrite();
	}
	static void OnDelWrite(void *pvthis)
	{
		auto pthis = reinterpret_cast<RedisDriver*>(pvthis);
		std::cout << "OnDelWrite()" << pthis->m_write.native_handle() << std::endl;

		if (pthis->m_write.is_open())
		{
			boost::system::error_code ec;
			pthis->m_write.cancel(ec);
			pthis->m_write.release();
		}
	}
	static void OnCleanUp(void *pvthis)
	{
		OnDelRead(pvthis);
		OnDelWrite(pvthis);
	}

private:
	boost::asio::io_context& m_bic;
	boost::asio::ip::tcp::socket m_read, m_write;

public:
	redisAsyncContext       *m_ctx{redisAsyncConnect("localhost", 6379)};
};

void get_callback(redisAsyncContext *, void * r, void *)
{
    redisReply * reply = static_cast<redisReply *>(r);
    if (reply)
	    std::cout << "key: " << std::string_view{reply->str, (unsigned)reply->len} << std::endl;
}

int main()
{
	boost::asio::io_context ic;

	RedisDriver redis{ic};

	redisAsyncCommand(redis.m_ctx, nullptr, nullptr, "SET key 100");
	redisAsyncCommand(redis.m_ctx, get_callback, nullptr, "GET key");

	ic.run();

	return 0;
}