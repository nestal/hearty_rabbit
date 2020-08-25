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

#include <deque>
#include <string>
#include <iostream>

namespace hrb::postgres {

class Result
{
public:
	explicit Result(::PGresult* result = nullptr);
	Result(Result&& r) : m_result{std::exchange(r.m_result, nullptr)} {}
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

class Query
{
public:
	template <typename String, typename... Args>
	explicit Query(String&& query, const Args& ... args) : m_query{std::forward<String>(query)}
	{
		add(args...);
	}

	template <typename Function>
	auto get(Function&& func) const
	{
		std::vector<const char*> values;
		std::vector<int> sizes;
		std::vector<int> formats;

		for (auto& arg : m_args)
		{
			values.push_back(arg.value.data());
			sizes.push_back(arg.value.size());
			formats.push_back(arg.is_text ? 0 : 1);
		}
		return func(m_query, m_args.size(), values.data(), sizes.data(), formats.data());
	}

private:
	void add()
	{
	}

	template <typename FirstArg, typename ... NextArgs>
	void add(const FirstArg& first, const NextArgs& ... next)
	{
		auto& arg = m_args.emplace_back();

		// special handling for const char*
		if constexpr (std::is_same_v<std::decay_t<decltype(first)>, const char*>)
		{
			arg.value   = first;
			arg.is_text = true;
		}

		// for string, string_view, const_buffer, ObjectID etc
		else
		{
			arg.value.assign(
				reinterpret_cast<const char*>(std::data(first)),
				std::size(first) * sizeof(*std::data(first))
			);

			if constexpr (!std::is_same_v<std::decay_t<decltype(std::data(first))>, const char*>)
				arg.is_text = false;
		}

		add(next...);
	}

private:
	std::string m_query;
	struct Arg
	{
		std::string value;
		bool        is_text{true};
	};
	std::vector<Arg>  m_args;
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

	template <typename ResultHandler, typename... Args>
	void query(const char *query, ResultHandler&& handler, Args ... args)
	{
		m_outstanding.emplace_back(Query{query, std::forward<Args>(args)...}, std::forward<ResultHandler>(handler));
		try_send_next_query();
	}

private:
	void try_send_next_query()
	{
		if (::PQisBusy(m_conn) || m_outstanding.empty())
			return;

		if (m_outstanding.front().query.get(
				[this](auto&& query, std::size_t size, const char* const* values, const int* sizes, const int* formats)
				{
					return ::PQsendQueryParams(m_conn, query.c_str(), static_cast<int>(size), nullptr, values, sizes, formats, 0);
				}
			))
		{
			async_read(std::move(m_outstanding.front().handler));
		}
		else
		{
			std::cout << "PQsendQueryParams() error: " << ::PQerrorMessage(m_conn) << std::endl;
			m_outstanding.front().handler(Result{});
		}

		m_outstanding.pop_front();
	}

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
			{
				::PGresult* result{};
				while ((result = ::PQgetResult(m_conn)) != nullptr)
				{
					handler(Result{result});
				}

				try_send_next_query();
			}
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

	struct OutstandingQuery
	{
		Query                       query;
		std::function<void(Result)> handler;

		template <typename Handler>
		OutstandingQuery(Query&& q, Handler&& h) : query{std::move(q)}, handler{std::forward<Handler>(h)}
		{
		}
	};
	std::deque<OutstandingQuery>    m_outstanding;
};

} // end of namespace hrb::postgres
