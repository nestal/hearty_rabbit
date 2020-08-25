/*
	Copyright © 2020 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 25/8/2020.
//

#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <postgresql/libpq-fe.h>

#include <string>

namespace hrb::postgres {

class Result
{
public:
	explicit Result(::PGresult* result = nullptr);
	Result(Result&&) = default;
	Result(const Result&) = delete;
	Result& operator=(Result&&) = default;
	Result& operator=(const Result&) = delete;
	~Result();

	auto fields() const {return ::PQnfields(m_result);}
	auto tuples() const {return ::PQntuples(m_result);}

private:
	::PGresult* m_result;
};

class Session
{
public:
	explicit Session(boost::asio::io_context& ioc);
	~Session();

	Session(Session&&) = default;
	Session(const Session&) = delete;
	Session& operator=(Session&&) = default;
	Session& operator=(const Session&) = delete;

	void open(const std::string& connection_string);

	template <typename ResultHandler>
	void query(const char *query, ResultHandler&& handler)
	{
		if (::PQsendQuery(m_conn, query))
			async_read(std::forward<ResultHandler>(handler));
		else
			std::forward<ResultHandler>(handler)(Result{});
	}

private:
	template <typename ResultHandler>
	void async_read(ResultHandler&& handler)
	{
		m_socket.async_wait(
			boost::asio::ip::tcp::socket::wait_read,
			[this, handler = std::forward<ResultHandler>(handler)](auto ec) mutable
			{
				on_read_ready(ec, std::forward<ResultHandler>(handler));
			}
		);
	}

	template <typename ResultHandler>
	void on_read_ready(boost::system::error_code ec, ResultHandler&& handler)
	{
		if (::PQconsumeInput(m_conn))
		{
			if (!::PQisBusy(m_conn))
				handler(Result{::PQgetResult(m_conn)});
			else
				async_read(std::forward<ResultHandler>(handler));
		}
	}

private:
	::PGconn*                       m_conn{nullptr};
	boost::asio::ip::tcp::socket    m_socket;
};

} // end of namespace hrb::postgres
