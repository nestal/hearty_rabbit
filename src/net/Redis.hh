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

#include "util/RepeatingTuple.hh"

#include <boost/asio.hpp>
#include <boost/iterator/iterator_adaptor.hpp>

#include <hiredis/hiredis.h>
#include <hiredis/async.h>

#include <deque>
#include <memory>
#include <string>

namespace hrb {
namespace redis {

// Error enum
enum class Error
{
	ok = REDIS_OK,
	io = REDIS_ERR_IO,
	eof = REDIS_ERR_EOF,
	protocol = REDIS_ERR_PROTOCOL,
	oom = REDIS_ERR_OOM,
	other = REDIS_ERR_OTHER,

	// other logical errors
	command_error = 1000,
	field_not_found
};

std::error_code make_error_code(Error err);
const std::error_category& redis_error_category();

class Reply
{
public:
	explicit Reply(redisReply *r = nullptr) noexcept;
	Reply(const Reply&) = delete;
	Reply(Reply&& other) = default ;
	~Reply() = default;

	Reply& operator=(const Reply&) = delete;
	Reply& operator=(Reply&& other) = default ;
	void swap(Reply& other) noexcept ;

	class iterator;
	iterator begin() const;
	iterator end() const;

	bool is_string() const {return m_reply->type == REDIS_REPLY_STRING;}

	std::string_view as_string() const noexcept;
	std::string_view as_status() const noexcept;
	std::string_view as_error() const noexcept;
	std::string_view as_any_string() const noexcept;

	explicit operator bool() const noexcept ;

	long as_int() const noexcept;
	long to_int() const noexcept;

	Reply as_array(std::size_t i) const noexcept;
	Reply as_array(std::size_t i, std::error_code& ec) const noexcept;
	Reply operator[](std::size_t i) const noexcept;
	std::size_t array_size() const noexcept;

	template <typename Func>
	void foreach_kv_pair(Func&& func)
	{
		for (auto i = 0U; i+1 < array_size() ; i += 2)
			func(as_array(i).as_string(), as_array(i+1));
	}

	template <typename... Field>
	auto map_kv_pair(Field... fields) const
	{
		typename RepeatingTuple<Reply, sizeof...(fields)>::type result;
		for (auto i = 0U; i+1 < array_size() ; i += 2)
			match_field(result, as_array(i).as_string(), as_array(i+1), fields...);
		return result;
	}

	template <std::size_t count>
	auto as_tuple(std::error_code& ec) const
	{
		return as_tuple_impl(ec, std::make_index_sequence<count>{});
	}

private:
	template <std::size_t... index>
	auto as_tuple_impl(std::error_code& ec, std::index_sequence<index...>) const
	{
		return std::make_tuple(as_array(index, ec)...);
	}

private:
	struct Deleter {void operator()(::redisReply*) const noexcept; };
	std::unique_ptr<::redisReply, Deleter> m_reply{};
};

class Reply::iterator : public boost::iterator_adaptor<
	iterator,
	::redisReply* const*,
	const Reply,
	boost::use_default,
	const Reply,
	boost::use_default
>
{
public:
	explicit iterator(::redisReply * const*elements = nullptr) : iterator_adaptor{elements} {}

 private:
	friend class boost::iterator_core_access;
	Reply dereference() const { return Reply(base() ? *base() : nullptr); }
};

class CommandString
{
public:
	template <typename... Args>
	CommandString(Args... args) : m_length{::redisFormatCommand(&m_cmd, args...)}
	{
		if (m_length < 0)
			throw std::logic_error("invalid command string");
	}
	CommandString(CommandString&& other) noexcept ;
	CommandString(const CommandString&) = delete;
	~CommandString();
	CommandString& operator=(CommandString&& other) noexcept ;
	CommandString& operator=(const CommandString&) = delete;

	void swap(CommandString& other) noexcept ;

	char* get() const {return m_cmd;}
	std::size_t length() const {return static_cast<std::size_t>(m_length);}
	auto buffer() const {return boost::asio::buffer(m_cmd, length());}

private:
	char    *m_cmd{};
	int     m_length{};
};

class Connection;
std::shared_ptr<Connection> connect(
	boost::asio::io_context& bic,
	const boost::asio::ip::tcp::endpoint& remote = boost::asio::ip::tcp::endpoint{
		boost::asio::ip::make_address("127.0.0.1"),
		6379
	}
);

class Connection : std::enable_shared_from_this<Connection>
{
private:
	struct Token {};

public:
	friend std::shared_ptr<Connection> connect(
		boost::asio::io_context& bic,
		const boost::asio::ip::tcp::endpoint& remote
	);

	explicit Connection(
		Token,
		boost::asio::io_context& ioc,
		const boost::asio::ip::tcp::endpoint& remote
	);

	Connection(Connection&&) = delete;
	Connection(const Connection&) = delete;
	~Connection();

	Connection& operator=(Connection&&) = delete;
	Connection& operator=(const Connection&) = delete;

	template <typename Callback, typename... Args>
	void command(Callback&& callback, Args... args)
	{
		try
		{
			do_write(
				CommandString{args...},
				[cb=std::make_shared<Callback>(std::forward<Callback>(callback))](auto&& r, auto ec)
				{
					(*cb)(std::move(r), std::move(ec));
				}
			);
		}
		catch (std::logic_error&)
		{
			callback(Reply{}, std::error_code{Error::protocol});
		}
	}

	boost::asio::io_context& get_io_context() {return m_ioc;}
	void disconnect() ;

private:
	using Completion = std::function<void(Reply, std::error_code)>;

	void do_write(CommandString&& cmd, Completion&& completion);
	void do_read();
	void on_read(boost::system::error_code ec, std::size_t bytes);

private:
	boost::asio::io_context& m_ioc;
	boost::asio::ip::tcp::socket m_socket;
	boost::asio::strand<boost::asio::io_context::executor_type> m_strand;

	char m_read_buf[4096];

	std::deque<Completion> m_callbacks;

	// This is not exception safe so put it in the last.
	::redisReader *m_reader{::redisReaderCreate()};
};

}} // end of namespace

namespace std
{
	template <> struct is_error_code_enum<hrb::redis::Error> : true_type {};
}
