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
#include <iostream>

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
//	[[nodiscard]] std::string_view status() const {return ::PQcmdStatus(m_result);}
	[[nodiscard]] std::string_view status() const {return ::PQresStatus(::PQresultStatus(m_result));}

private:
	::PGresult* m_result;
};

class QueryParams
{
public:
	template <typename... Args>
	explicit QueryParams(const Args& ... args)
	{
		add(args...);
	}


	[[nodiscard]] auto values() const {return m_values.data();}
	[[nodiscard]] auto sizes() const {return m_sizes.data();}
	[[nodiscard]] auto formats() const {return m_formats.data();}
	[[nodiscard]] int size() const {return static_cast<int>(m_sizes.size());}

private:
	void add()
	{
	}

	template <typename FirstArg, typename ... NextArgs>
	void add(const FirstArg& arg, const NextArgs& ... next)
	{
		const char* value{};
		int   size{};
		int   format{};

		// special handling for const char*
		if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, const char*>)
		{
			value  = arg;
			size   = std::strlen(arg);
			format = 0;
		}

		// for string, string_view, const_buffer, ObjectID etc
		else
		{
			value = reinterpret_cast<const char*>(std::data(arg));
			size  = static_cast<int>(std::size(arg) * sizeof(*value));

			if constexpr (!std::is_same_v<std::decay_t<decltype(std::data(arg))>, const char*>)
				format = 1;
		}

		m_values.push_back(value);
		m_sizes.push_back(size);
		m_formats.push_back(format);

		add(next...);
	}

private:
	std::vector<const char*>    m_values;
	std::vector<int>            m_sizes;
	std::vector<int>            m_formats;
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
		{
			std::cout << "PQsendQuery() error: " << ::PQerrorMessage(m_conn) << std::endl;
			std::forward<ResultHandler>(handler)(Result{});
		}
	}

	template <typename ResultHandler, typename... Args>
	void query(const char *query, ResultHandler&& handler, Args ... args)
	{
		QueryParams params{std::forward<Args>(args)...};

		if (::PQsendQueryParams(m_conn, query, params.size(), nullptr, params.values(), params.sizes(), params.formats(), 0))
			async_read(std::forward<ResultHandler>(handler));
		else
		{
			std::cout << "PQsendQueryParams() error: " << ::PQerrorMessage(m_conn) << std::endl;
			std::forward<ResultHandler>(handler)(Result{});
		}
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
		else
		{
			std::cout << "PQconsumeInput() error: " << ::PQerrorMessage(m_conn) << std::endl;
			std::forward<ResultHandler>(handler)(Result{});
		}
	}

private:
	::PGconn*                       m_conn{nullptr};
	boost::asio::ip::tcp::socket    m_socket;
};

} // end of namespace hrb::postgres
