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

class RedisDriver
{
public:
	struct Error : virtual Exception {};
	using ErrorMsg = boost::error_info<struct tag_error_msg, std::string>;

public:
	explicit RedisDriver(boost::asio::io_context& bic, const std::string& host, unsigned short port);
	~RedisDriver();

	template <typename Callback, typename... Args>
	void command(Callback&& callback, const char *fmt, Args... args)
	{
		using CallbackType = std::remove_reference_t<Callback>;

		auto callback_ptr = std::make_unique<CallbackType>(std::forward<Callback>(callback));
		auto r = ::redisAsyncCommand(m_ctx, [](redisAsyncContext *, void *reply, void *pv_callback)
		{
			std::unique_ptr<CallbackType> callback{static_cast<CallbackType*>(pv_callback)};
			(*callback)(static_cast<redisReply*>(reply));
		}, callback_ptr.release(), fmt, args...);
	}

private:
	static redisAsyncContext* connect(const std::string& host, unsigned short port);

	void do_read();
	void do_write();

private:
	boost::asio::io_context& m_bic;
	boost::asio::ip::tcp::socket m_read, m_write;

	redisAsyncContext *m_ctx{};
};

} // end of namespace
