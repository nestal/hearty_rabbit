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
#include "util/BufferView.hh"

#include <boost/asio.hpp>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/range/iterator_range.hpp>

#include <hiredis/hiredis.h>

#include <deque>
#include <memory>
#include <string>
#include <type_traits>
#include <mutex>

namespace hrb::redis {

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

}

namespace std
{
	template <> struct is_error_code_enum<hrb::redis::Error> : true_type {};
}

namespace hrb::redis {

class Reply
{
public:
	explicit Reply(redisReply *r = nullptr) noexcept;
	Reply(const Reply&) = default;
	Reply(Reply&& other) = default ;
	~Reply() = default;

	Reply& operator=(const Reply&) = default;
	Reply& operator=(Reply&& other) = default ;
	void swap(Reply& other) noexcept ;

	using reference = std::vector<Reply>::reference;
	using iterator = std::vector<Reply>::const_iterator;
	using const_iterator = std::vector<Reply>::const_iterator;
	[[nodiscard]] iterator begin() const;
	[[nodiscard]] iterator end() const;

	[[nodiscard]] bool is_string() const {return m_reply->type == REDIS_REPLY_STRING;}
	[[nodiscard]] bool is_nil() const {return m_reply->type == REDIS_REPLY_NIL;}

	[[nodiscard]] std::string_view as_string() const noexcept;
	[[nodiscard]] std::string_view as_status() const noexcept;
	[[nodiscard]] std::string_view as_error() const noexcept;
	[[nodiscard]] std::string_view as_any_string() const noexcept;
	[[nodiscard]] boost::asio::const_buffer as_buffer() const noexcept;
	[[nodiscard]] BufferView as_buffer_view() const noexcept;
	[[nodiscard]] auto type() const {return m_reply->type;}
	[[nodiscard]] std::size_t length() const;

	explicit operator bool() const noexcept ;

	[[nodiscard]] long as_int() const noexcept;
	[[nodiscard]] long to_int() const noexcept;

	[[nodiscard]] Reply as_array(std::size_t i) const noexcept;
	[[nodiscard]] Reply as_array(std::size_t i, std::error_code& ec) const noexcept;
	[[nodiscard]] Reply operator[](std::size_t i) const noexcept;
	[[nodiscard]] std::size_t array_size() const noexcept;

	class KeyValue
	{
	public:
		explicit KeyValue(iterator current) : m_current{current} {}

		[[nodiscard]] auto key() const {return m_current->as_string();}
		[[nodiscard]] auto value() const {auto i = m_current; return *++i;}

	private:
		iterator m_current;
	};
	class kv_iterator : public boost::iterator_adaptor<
	    kv_iterator,
		iterator,
		KeyValue,
		boost::random_access_traversal_tag,
		KeyValue
	>
	{
	public:
		kv_iterator() = default;
		explicit kv_iterator(iterator it) : kv_iterator::iterator_adaptor_{it} {}
	private:
		friend class boost::iterator_core_access;
		void increment() {this->base_reference() += 2;}
		[[nodiscard]] auto dereference() const {return KeyValue{this->base()};}
		[[nodiscard]] auto distance_to(kv_iterator other) const
		{
			return (other.base() - base()) / 2;
		}
		void advance(typename iterator_adaptor::difference_type n)
		{
			this->base_reference() += n*2;
		}
	};

	[[nodiscard]] auto kv_pairs() const
	{
		return boost::iterator_range<kv_iterator>{
			kv_iterator{begin()},
			kv_iterator{end()}
		};
	}

	// Return a tuple of replies, one for each field in the parameter list
	template <typename... Field>
	auto map_kv_pair(Field... fields) const
	{
		typename RepeatingTuple<Reply, sizeof...(fields)>::type result;
		for (auto i = 0U; i+1 < array_size() ; i += 2)
			match_field(result, as_array(i).as_string(), as_array(i+1), fields...);
		return result;
	}

	/// Return a tuple of the first \a count replies in the array.
	template <std::size_t count>
	auto as_tuple(std::error_code& ec) const
	{
		return as_tuple_impl(ec, std::make_index_sequence<count>{});
	}

	char* as_mutable_string();

private:
	template <std::size_t... index>
	auto as_tuple_impl(std::error_code& ec, std::index_sequence<index...>) const
	{
		return std::make_tuple(as_array(index, ec)...);
	}

private:
	std::shared_ptr<::redisReply> m_reply;

	// If m_reply represents an array, each element in the "element" array in redisReply
	// will be moved to m_array.
	std::vector<Reply> m_array;
};

class ReplyReader
{
public:
	enum class Result {ok, error, not_ready};

public:
	ReplyReader() = default;
	ReplyReader(ReplyReader&&) = default;
	ReplyReader(const ReplyReader&) = delete;
	~ReplyReader() = default;
	ReplyReader& operator=(ReplyReader&&) = default;
	ReplyReader& operator=(const ReplyReader&) = delete;

	void feed(const char *data, std::size_t size);
	std::tuple<Reply, Result> get();

private:
	struct Deleter {void operator()(::redisReader*) const noexcept; };
	std::unique_ptr<::redisReader, Deleter> m_reader{::redisReaderCreate()};
};

class CommandString
{
public:
	CommandString() = default;

	/// The first argument MUST be the command string. For security
	/// reason, this class does not accept std::string and const char*.
	/// It expects a hard-coded string literal. However in C++ we can't
	/// make it mandatory. We can only specify a const char array.
	template <std::size_t N, typename... Args>
	explicit CommandString(const char (&cmd)[N], Args... args) :
		m_length{::redisFormatCommand(&m_cmd, cmd, args...)}
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

	[[nodiscard]] auto get() const {return m_cmd;}
	[[nodiscard]] auto length() const {return static_cast<std::size_t>(m_length);}
	[[nodiscard]] auto buffer() const {return boost::asio::buffer(m_cmd, length());}
	[[nodiscard]] std::string_view str() const {return {m_cmd, length()};}

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

class Pool;
class PoolBase
{
public:
	virtual void dealloc(boost::asio::ip::tcp::socket socket) = 0;
};

class Connection : public std::enable_shared_from_this<Connection>
{
public:
	using Completion = std::function<void(Reply, std::error_code)>;

public:
	explicit Connection(
		PoolBase& parent,
		boost::asio::ip::tcp::socket socket
	);

	Connection(Connection&&) = delete;
	Connection(const Connection&) = delete;
	~Connection();

	Connection& operator=(Connection&&) = delete;
	Connection& operator=(const Connection&) = delete;

	template <
		typename Callback,
		std::size_t N,
		typename... Args
	>
	typename std::enable_if_t<
		std::is_invocable<Callback, Reply, std::error_code>::value &&
		std::is_copy_constructible<Callback>::value
	>
	command(Callback&& callback, const char (&cmd)[N], Args... args)
	{
		try
		{
			command(std::forward<Callback>(callback), CommandString{cmd, args...});
		}
		catch (std::logic_error&)
		{
			callback(Reply{}, std::error_code{Error::protocol});
		}
	}

	// Only enable this template if Callback is a copy-constructible
	// function-like type that takes two argument: Reply, std::error_code.
	// If Callback is copy-constructible, we can copy it to the lambda
	// capture directly. The other overload, which uses a shared_ptr to
	// store the callback, does not require the callback to be copy-
	// constructible, but will have more overhead from the shared_ptr.
	template <typename Callback>
	typename std::enable_if_t<
		std::is_invocable_v<Callback, Reply, std::error_code> &&
		std::is_copy_constructible_v<Callback>
	>
	command(Callback&& callback, CommandString&& command)
	{
		do_write(std::move(command),
			[
				callback=std::forward<Callback>(callback),
				self=shared_from_this()
			](auto&& r, auto ec) mutable
			{
				callback(std::forward<decltype(r)>(r), std::move(ec));
			}
		);
	}

	template <typename Callback>
	typename std::enable_if_t<
		std::is_invocable_v<Callback, Reply, std::error_code> &&
		std::is_copy_constructible_v<Callback>
	>
	command(Callback&& callback, std::vector<CommandString>&& commands)
	{
		for (auto&& cmd : commands)
		{
			if (&cmd == &commands.back())
				do_write(std::move(cmd), std::forward<Callback>(callback));
			else
				do_write(std::move(cmd), [](auto&&, auto){});
		}
	}

	template <
		typename Callback,
		std::size_t N,
		typename... Args
	>

	// Only enable this template if Callback is a move-constructible
	// function-like type that takes two argument: Reply, std::error_code.
	// This function will move the passed callback into a shared_ptr,
	// which will be stored in a lambda capture.
	typename std::enable_if_t<
		std::is_invocable_v<Callback, Reply, std::error_code> &&
		std::is_move_constructible_v<Callback> &&
		!std::is_copy_constructible_v<Callback>
	>
	command(Callback&& callback, const char (&cmd)[N], Args... args)
	{
		try
		{
			do_write(
				CommandString{cmd, args...},
				[
					cb=std::make_shared<std::remove_reference_t<Callback>>(std::forward<Callback>(callback)),
					self=shared_from_this()
				](auto&& r, auto ec) mutable
				{
					(*cb)(std::forward<decltype(r)>(r), std::move(ec));
				}
			);
		}
		catch (std::logic_error&)
		{
			callback(Reply{}, std::error_code{Error::protocol});
		}
	}

	template <std::size_t N, typename... Args>
	void command(const char (&cmd)[N], Args... args)
	{
		try
		{
			do_write(CommandString{cmd, args...}, [](auto&&, auto&&){});
		}
		catch (std::logic_error&)
		{
		}
	}

	void do_write(CommandString&& cmd, Completion&& completion);

private:
	// must not call disconnect() inside the callbacks in m_callbacks
	void disconnect(std::error_code ec) ;

private:
	void do_read();
	void on_read(boost::system::error_code ec, std::size_t bytes);
	void on_exec_transaction(Reply&& reply, std::error_code ec);

private:
	boost::asio::ip::tcp::socket m_socket;

	char m_read_buf[8*1024]{};

	std::deque<Completion> m_callbacks;
	std::vector<Completion> m_queued_callbacks;

	ReplyReader m_reader;
	PoolBase&   m_parent;
};

class Pool : public PoolBase
{
public:
	Pool(boost::asio::io_context& ioc, const boost::asio::ip::tcp::endpoint& remote);

	std::shared_ptr<Connection> alloc();
	void dealloc(boost::asio::ip::tcp::socket socket) override;

private:
	boost::asio::ip::tcp::socket get_sock();

private:
	boost::asio::io_context&                    m_ioc;
	boost::asio::ip::tcp::endpoint              m_remote;
	std::vector<boost::asio::ip::tcp::socket>   m_socks;

	std::mutex  m_mx;
};

} // end of namespace
