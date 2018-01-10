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

class RedisDriver
{
public:
	RedisDriver(boost::asio::io_context& bic) :
		m_bic{bic},
		m_read{m_bic},
		m_write{m_bic}
	{
		m_ctx->ev.data = this;
		m_ctx->ev.addRead = RedisDriver::OnAddRead;
		m_ctx->ev.delRead = RedisDriver::OnDelRead;
		m_ctx->ev.addWrite = RedisDriver::OnAddWrite;
		m_ctx->ev.delWrite = RedisDriver::OnDelWrite;
		m_ctx->ev.cleanup = RedisDriver::OnCleanUp;
	}

private:
	static void OnAddRead(void *pvthis)
	{
		auto pthis = reinterpret_cast<RedisDriver*>(pvthis);
		pthis->m_read.assign(boost::asio::ip::tcp::v4(), pthis->m_ctx->c.fd);
		pthis->DoRead();
	}
	void DoRead()
	{
		m_read.async_wait(boost::asio::socket_base::wait_read, [this](const boost::system::error_code& ec2)
		{
			OnRead(ec2);
		});
	}
	void OnRead(const boost::system::error_code& ec)
	{
		redisAsyncHandleRead(m_ctx);

		if (!ec)
			DoRead();
	}
	static void OnDelRead(void *pvthis)
	{
		auto pthis = reinterpret_cast<RedisDriver*>(pvthis);

		boost::system::error_code ec;
		pthis->m_read.cancel(ec);
	}
	static void OnAddWrite(void *pvthis)
	{
		auto pthis = reinterpret_cast<RedisDriver*>(pvthis);

	}
	static void OnDelWrite(void *pvthis)
	{
		auto pthis = reinterpret_cast<RedisDriver*>(pvthis);

	}
	static void OnCleanUp(void *pvthis)
	{
		auto pthis = reinterpret_cast<RedisDriver*>(pvthis);

	}

private:
	boost::asio::io_context& m_bic;
	boost::asio::ip::tcp::socket m_read, m_write;
	redisAsyncContext       *m_ctx{redisAsyncConnect("localhost", 6379)};
};

int main()
{
	auto c = redisConnect("127.0.0.1", 6379);
	if (!c || c->err)
	{
	    if (c)
	    {
	        printf("Error: %s\n", c->errstr);
	        // handle error
	    }
	    else
	    {
	        printf("Can't allocate redis context\n");
	    }
	}

	auto reply = static_cast<redisReply*>(redisCommand(c, "set mycounter 1"));
	switch (reply->type)
	{
		case REDIS_REPLY_STATUS:
			std::cout << "Return status: " << std::string_view(reply->str, reply->len) << std::endl;
			break;
		case REDIS_REPLY_STRING:
			std::cout << "Return string: " << std::string_view(reply->str, reply->len) << std::endl;
			break;
	}

	redisFree(c);

	return 0;
}