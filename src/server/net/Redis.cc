/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/11/18.
//


#include "Redis.hh"

#include "util/Backtrace.hh"
#include "util/Log.hh"

#include "util/Error.hh"

#include <boost/asio/strand.hpp>
#include <boost/exception/info.hpp>
#include <boost/exception/errinfo_api_function.hpp>
#include <boost/system/error_code.hpp>

#include <cassert>

namespace hrb {
namespace redis {

std::shared_ptr<Connection> connect(
	boost::asio::io_context& bic,
	const boost::asio::ip::tcp::endpoint& remote
)
{
	struct Wrapper
	{
		Pool pool;
		std::shared_ptr<Connection> conn;

		Wrapper(
			boost::asio::io_context& bic,
			const boost::asio::ip::tcp::endpoint& remote
		) : pool{bic, remote}, conn{pool.alloc()}
		{
		}
	};

	auto p = std::make_shared<Wrapper>(bic, remote);
	return std::shared_ptr<Connection>(p, p->conn.get());
}

Connection::Connection(
	PoolBase& parent,
	boost::asio::ip::tcp::socket socket
) :
	m_socket{std::move(socket)},
	m_parent{parent}
{
}

Connection::~Connection()
{
	// no point to reuse a closed socket
	if (m_socket.is_open())
		m_parent.dealloc(std::move(m_socket));
}

void Connection::do_write(CommandString&& cmd, Completion&& completion)
{
	// In some cases, the redis reply will be received before the async_write() callback
	// is called. In other words, the write-callback of sending the redis command is
	// called even _after_ we receive redis' reply to that command.
	// If we en-queue the completion routine in the write-callback, it is possible that
	// the completion routine is not queued yet by the time the reply arrives.
	// In order to avoid this, we have to en-queue the completion routine before sending
	// the command to redis. It is impossible to receive a reply before the command is
	// sent.
	m_callbacks.push_back(std::move(completion));
	if (m_callbacks.size() == 1)
		do_read();

	auto buffer = cmd.buffer();
	async_write(
		m_socket,
		buffer,
		[this, cmd=std::move(cmd), self=shared_from_this()](auto ec, std::size_t bytes)
		{
			if (ec)
			{
				Log(LOG_WARNING, "redis write error %1% %2%", ec, ec.message());
				disconnect(Error::protocol);
			}
		}
	);
}

void Connection::do_read()
{
	m_socket.async_read_some(
		boost::asio::buffer(m_read_buf),
		[this, self=shared_from_this()](auto ec, auto read){ on_read(ec, read); }
	);
}
void Connection::on_read(boost::system::error_code ec, std::size_t bytes)
{
	assert(!m_callbacks.empty());
	if (!ec)
	{
		m_reader.feed(&m_read_buf[0], bytes);

		auto [reply, result] = m_reader.get();

		// Extract all replies from the
		while (!m_callbacks.empty() && result == ReplyReader::Result::ok)
		{
			// When we are in the middle of a transaction, the commands will be queued
			// by redis. These queued commands will be executed when the "EXEC" command
			// is sent. We cannot call the callbacks for these queued commands. Instead,
			// we move them to m_queued_callback so that they will be executed when "EXEC".
			if (reply.as_status() == "QUEUED")
				m_queued_callbacks.push_back(std::move(m_callbacks.front()));

			// reply is not QUEUED but inside a transaction, that means the transaction
			// is just executed.
			else if (!m_queued_callbacks.empty())
				on_exec_transaction(std::move(reply), std::error_code{ec.value(), ec.category()});

			else
				m_callbacks.front()(std::move(reply), std::error_code{ec.value(), ec.category()});

			m_callbacks.pop_front();

			std::tie(reply, result) = m_reader.get();
		}

		if (result == ReplyReader::Result::ok)
		{
			assert(m_callbacks.empty());
			Log(LOG_WARNING, "Redis sends more replies than requested. Disconnecting. %1% %2%", reply.type(), reply.as_any_string());

			// clear callback
			result = ReplyReader::Result::error;
		}

		// Report parse error as protocol errors in the callbacks
		while (result == ReplyReader::Result::error && !m_callbacks.empty())
		{
			m_callbacks.front()(Reply{}, std::error_code{Error::protocol});
			m_callbacks.pop_front();
		}

		if (result == ReplyReader::Result::error)
		{
			Log(LOG_WARNING, "Redis reply parse error. Disconnecting.");
			disconnect(Error::protocol);
		}

		// Keep reading until all outstanding commands are finished.
		// This will keep the io_context::run() from returning.
		else if (!m_callbacks.empty())
			do_read();
	}
	else
	{
		Log(LOG_WARNING, "Redis read error: %1% (%2%). Disconnecting.", ec, ec.message());
		disconnect(ec);
	}
}

void Connection::disconnect(std::error_code ec)
{
	// clean up all outstanding callbacks
	// must not call this function inside any of these callbacks!!
	for (auto&& cb : m_queued_callbacks)
		cb(Reply{}, ec ? ec : std::error_code{Error::io});
	m_queued_callbacks.clear();

	for (auto&& cb : m_callbacks)
		cb(Reply{}, ec ? ec : std::error_code{Error::io});
	m_callbacks.clear();

	m_socket.close();
}

void Connection::on_exec_transaction(Reply&& reply, std::error_code ec)
{
	assert(!m_queued_callbacks.empty());

	// transaction failed (i.e. WATCH error) or "DISCARD"
	if (reply.is_nil() || reply.as_status() == "OK")
	{
		for (auto&& callback : m_queued_callbacks)
			if (callback)
				callback(Reply{reply}, hrb::Error::redis_transaction_aborted);
	}

	// transaction executed
	else if (reply.array_size() == m_queued_callbacks.size())
	{
		auto callback = m_queued_callbacks.begin();
		for (auto&& transaction_reply : reply)
		{
			assert(!m_queued_callbacks.empty());
			assert(callback != m_queued_callbacks.end());

			if (*callback)
				(*callback)(Reply{transaction_reply}, ec);
			callback++;
		}
	}

	// something is wrong
	else
	{
		Log(LOG_CRIT, "assertion failed: redis sends bad reply %1% %2%", reply.array_size(), m_queued_callbacks.size());
		assert(false);
	}

	// run the callback for the "EXEC" command
	assert(!m_callbacks.empty());
	m_callbacks.front()(std::move(reply), ec);

	m_queued_callbacks.clear();
}

Reply::Reply(::redisReply *r) noexcept :
	m_reply{r, [](::redisReply *r){::freeReplyObject(r);}}
{
	// Special handling for arrays
	for (std::size_t i = 0 ; m_reply && m_reply->type == REDIS_REPLY_ARRAY && i < m_reply->elements; i++)
	{
		// Steal the element pointer and assign it to the shared_ptr of
		// our vector. Basically takes the ownership of the array element.
		m_array.emplace_back(m_reply->element[i]);
		m_reply->element[i] = nullptr;
	}
}

void Reply::swap(Reply& other) noexcept
{
	m_reply.swap(other.m_reply);
	m_array.swap(other.m_array);
}

std::string_view Reply::as_string() const noexcept
{
	return (m_reply && m_reply->type == REDIS_REPLY_STRING) ? as_any_string() : std::string_view{};
}

std::string_view Reply::as_status() const noexcept
{
	return (m_reply && m_reply->type == REDIS_REPLY_STATUS) ? as_any_string() : std::string_view{};
}

std::string_view Reply::as_error() const noexcept
{
	return (m_reply && m_reply->type == REDIS_REPLY_ERROR) ? as_any_string() : std::string_view{};
}

std::string_view Reply::as_any_string() const noexcept
{
	return (m_reply && (
		m_reply->type == REDIS_REPLY_STRING ||
		m_reply->type == REDIS_REPLY_STATUS ||
		m_reply->type == REDIS_REPLY_ERROR
	)) ?
		std::string_view{m_reply->str, static_cast<std::size_t>(m_reply->len)} : std::string_view{};
}

boost::asio::const_buffer Reply::as_buffer() const noexcept
{
	auto s = as_any_string();
	return {s.data(), s.size()};
}

Reply Reply::as_array(std::size_t i) const noexcept
{
	return m_reply && m_reply->type == REDIS_REPLY_ARRAY && i < m_array.size() ?
		m_array[i] : Reply{};
}

Reply Reply::as_array(std::size_t i, std::error_code& ec) const noexcept
{
	// If there is already an error, do nothing and because we can't report error.
	if (ec || !m_reply)
		return Reply{};

	if (m_reply->type == REDIS_REPLY_ARRAY && i < m_array.size())
	{
		return m_array[i];
	}
	else
	{
		ec = Error::field_not_found;
		return Reply{};
	}
}

Reply::iterator Reply::begin() const
{
	return m_array.begin();
}

Reply::iterator Reply::end() const
{
	return m_array.end();
}

Reply Reply::operator[](std::size_t i) const noexcept
{
	return as_array(i);
}

std::size_t Reply::array_size() const noexcept
{
	return m_reply && m_reply->type == REDIS_REPLY_ARRAY ? m_array.size() : 0ULL;
}

long Reply::as_int() const noexcept
{
	return m_reply && m_reply->type == REDIS_REPLY_INTEGER ? m_reply->integer : 0;
}

Reply::operator bool() const noexcept
{
	return m_reply && m_reply->type != REDIS_REPLY_ERROR;
}

long Reply::to_int() const noexcept
{
	auto s = as_any_string();
	return s.empty() ? 0 : std::stol(std::string{s});
}

char* Reply::as_mutable_string()
{
	return m_reply && m_reply->type == REDIS_REPLY_STRING ? m_reply->str : nullptr;
}

std::size_t Reply::length() const
{
	return m_reply && m_reply->type == REDIS_REPLY_STRING ? static_cast<std::size_t>(m_reply->len) : 0;
}

const std::error_category& redis_error_category()
{
	struct Cat : std::error_category
	{
		const char *name() const noexcept override { return "redis"; }

		std::string message(int ev) const override
		{
			switch (static_cast<Error>(ev))
			{
				case Error::ok: return "success";
				case Error::io: return "IO error";
				case Error::eof: return "EOF error";
				case Error::protocol: return "protocol error";
				case Error::oom: return "out-of-memory error";
				case Error::other: return "other error";
				case Error::command_error: return "command error";
				case Error::field_not_found: return "field not found";
				default: return "unknown error";
			}
		}
	};
	static const Cat cat{};
	return cat;
}

std::error_code make_error_code(Error err)
{
	return std::error_code(static_cast<int>(err), redis_error_category());
}

CommandString::CommandString(CommandString&& other) noexcept
{
	swap(other);
}

CommandString::~CommandString()
{
	::redisFreeCommand(m_cmd);
}

CommandString& CommandString::operator=(CommandString&& other) noexcept
{
	CommandString tmp{std::move(other)};
	swap(tmp);
	return *this;
}

void CommandString::swap(CommandString& other) noexcept
{
	std::swap(m_cmd, other.m_cmd);
	std::swap(m_length, other.m_length);
}

void ReplyReader::feed(const char *data, std::size_t size)
{
	::redisReaderFeed(m_reader.get(), data, size);
}

std::tuple<Reply, ReplyReader::Result> ReplyReader::get()
{
	::redisReply *reply{};
	auto result = ::redisReaderGetReply(m_reader.get(), (void**)&reply);
	return std::make_tuple(
		Reply{reply},
		result == REDIS_OK ? (reply ? Result::ok : Result::not_ready) : Result::error
	);
}

void ReplyReader::Deleter::operator()(::redisReader *reader) const noexcept
{
	::redisReaderFree(reader);
}

Pool::Pool(boost::asio::io_context& ioc, const boost::asio::ip::tcp::endpoint& remote) :
	m_ioc{ioc},
	m_remote{remote}
{
}

boost::asio::ip::tcp::socket Pool::get_sock()
{
	if (!m_socks.empty())
	{
		std::unique_lock<std::mutex> lock{m_mx};
		if (!m_socks.empty())
		{
			auto sock = std::move(m_socks.back());
			m_socks.pop_back();
			return sock;
		}
	}

	boost::asio::ip::tcp::socket sock{boost::asio::make_strand(m_ioc)};
	sock.connect(m_remote);

	return sock;
}

std::shared_ptr<Connection> Pool::alloc()
{
	return std::make_shared<Connection>(*this, get_sock());
}

void Pool::dealloc(boost::asio::ip::tcp::socket socket)
{
	std::unique_lock<std::mutex> lock{m_mx};
	m_socks.push_back(std::move(socket));
}

}} // end of namespace
