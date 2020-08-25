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
#include <libpq-fe.h>

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

	[[nodiscard]] auto fields() const {return ::PQnfields(m_result);}
	[[nodiscard]] auto tuples() const {return ::PQntuples(m_result);}

private:
	::PGresult* m_result;
};

class Session
{
public:
	Session(boost::asio::io_context& ioc, const std::string& connection_string);
	~Session();

	Session(Session&&) = default;
	Session(const Session&) = delete;
	Session& operator=(Session&&) = default;
	Session& operator=(const Session&) = delete;

	[[nodiscard]] std::string_view last_error() const;

	template <typename ResultHandler>
	void query(const char *query, ResultHandler&& handler)
	{
		if (::PQsendQuery(m_conn, query))
			async_read(std::forward<ResultHandler>(handler));
		else
			std::forward<ResultHandler>(handler)(Result{});
	}

	template <typename ResultHandler, typename... Args>
	void query(const char *query, ResultHandler&& handler, Args ... args)
	{
		std::vector<std::string> params{args...};

		std::vector<const char*> param_values;
		std::vector<int> param_sizes;
		for (auto& p : params)
		{
			param_values.push_back(p.data());
			param_sizes.push_back(static_cast<int>(p.size()));
		}

		if (::PQsendQueryParams(m_conn, query, params.size(), nullptr, param_values.data(), param_sizes.data(), nullptr, 0))
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
