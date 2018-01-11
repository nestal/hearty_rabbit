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

	template <typename Callback>
	void command(const std::string& command, Callback&& callback)
	{
		using CallbackType = std::remove_reference_t<Callback>;

		auto callback_ptr = std::make_unique<CallbackType>(std::forward<Callback>(callback));
		::redisAsyncCommand(m_ctx, [](redisAsyncContext *, void *reply, void *pv_callback)
		{
			std::unique_ptr<CallbackType> callback{static_cast<CallbackType*>(reply)};
			(*callback)(static_cast<redisReply*>(reply));
		}, callback_ptr.release(), command.c_str());
	}

private:
	static void OnAddRead(void *pvthis)
	{
		if (auto pthis = reinterpret_cast<RedisDriver*>(pvthis); !pthis->m_read.is_open())
			pthis->DoRead();
	}
	void DoRead()
	{
		m_read.assign(boost::asio::ip::tcp::v4(), m_ctx->c.fd);
		m_read.async_wait(boost::asio::socket_base::wait_read, [this](auto&& ec)
		{
			// release socket before calling redisAsyncHandleRead() because it may close the socket
			m_read.release();
			redisAsyncHandleRead(m_ctx);
		});
	}
	static void OnDelRead(void *pvthis)
	{
		if (auto pthis = reinterpret_cast<RedisDriver*>(pvthis); pthis->m_read.is_open())
		{
			boost::system::error_code ec;
			pthis->m_read.cancel(ec);
			pthis->m_read.release();
		}
	}
	static void OnAddWrite(void *pvthis)
	{
		if (auto pthis = reinterpret_cast<RedisDriver*>(pvthis); !pthis->m_write.is_open())
			pthis->DoWrite();
	}
	void DoWrite()
	{
		m_write.assign(boost::asio::ip::tcp::v4(), m_ctx->c.fd);
		m_write.async_wait(boost::asio::socket_base::wait_write, [this](auto&& ec)
		{
			m_write.release();
			redisAsyncHandleWrite(m_ctx);
		});
	}
	static void OnDelWrite(void *pvthis)
	{
		if (auto pthis = reinterpret_cast<RedisDriver*>(pvthis); pthis->m_write.is_open())
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

	redisAsyncContext       *m_ctx{redisAsyncConnect("localhost", 6379)};
};

int main()
{
	boost::asio::io_context ic;

	RedisDriver redis{ic};

	redis.command("SET key 100", [](auto) {});
	redis.command("GET key", [](auto reply)
	{
	    if (reply)
		    std::cout << "key: " << std::string_view{reply->str, (unsigned)reply->len} << std::endl;
	});

	ic.run();

	return 0;
}