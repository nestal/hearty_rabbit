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
#include "util/Error.hh"

#include <boost/exception/info.hpp>
#include <boost/exception/errinfo_api_function.hpp>
#include <boost/system/error_code.hpp>

#include <cassert>
#include <iostream>

namespace hrb {
namespace redis {

std::shared_ptr<Connection> connect(
	boost::asio::io_context& bic,
	const boost::asio::ip::tcp::endpoint& remote
)
{
	return std::make_shared<Connection>(Connection::Token{}, bic, remote);
}

Connection::Connection(
	Token,
	boost::asio::io_context& ioc,
	const boost::asio::ip::tcp::endpoint& remote
) : m_ioc{ioc}, m_socket{m_ioc}
{
	m_socket.connect(remote);
}

void Connection::do_write(CommandString&& cmd, Completion&& completion)
{
	auto buffer = cmd.buffer();
	async_write(
		m_socket,
		buffer,
		[this, cmd=std::move(cmd), completion=std::move(completion)](auto ec, std::size_t bytes) mutable
	{
		std::cout << ec.message() << ": command sent" << std::endl;

		if (!ec)
		{
			m_callbacks.push_back(std::move(completion));
			if (m_callbacks.size() == 1)
			{
				std::cout << "proceed to read: " << std::endl;
				do_read();
			}
			else
			{
				std::cout << "already have " << m_callbacks.size() << " commands pending. no need to read." << std::endl;
			}
		}
		else
			completion(Reply{}, std::error_code{ec.value(), ec.category()});
	});
}

void Connection::do_read()
{
	m_socket.async_read_some(
		boost::asio::buffer(m_read_buf),
		[this](auto ec, auto read){ on_read(ec, read); }
	);
}
void Connection::on_read(boost::system::error_code ec, std::size_t bytes)
{
	assert(!m_callbacks.empty());
	if (!ec)
	{
		::redisReaderFeed(m_reader, m_read_buf, bytes);

		::redisReply *reply{};
		auto result = ::redisReaderGetReply(m_reader, (void**)&reply);

		while (!m_callbacks.empty() && result == REDIS_OK && reply)
		{
			std::cout << "reply received!" << std::endl;
			m_callbacks.front()(Reply{reply}, std::error_code{ec.value(), ec.category()});
			m_callbacks.pop_front();

			result = ::redisReaderGetReply(m_reader, (void**)&reply);
		}

		// Keep reading until all outstanding commands are finished
		if (!m_callbacks.empty())
		{
			std::cout << "have " << m_callbacks.size() << " commands pending. proceed to read more." << std::endl;
			do_read();
		}
		else
		{
			std::cout << "no more commands pending. no need to read." << std::endl;
		}
	}
	else
	{
		std::cout << "oops! " << ec << " " << ec.message() << std::endl;
	}
}

Connection::~Connection()
{
	::redisReaderFree(m_reader);
}

void Connection::disconnect()
{
	m_socket.close();
}

Reply::Reply(const redisReply *r) noexcept :
	m_reply{r}
{
	static const redisReply empty{};
	if (!m_reply)
		m_reply = &empty;
}

std::string_view Reply::as_string() const noexcept
{
	return (m_reply->type == REDIS_REPLY_STRING) ? as_any_string() : std::string_view{};
}

std::string_view Reply::as_status() const noexcept
{
	return (m_reply->type == REDIS_REPLY_STATUS) ? as_any_string() : std::string_view{};
}

std::string_view Reply::as_error() const noexcept
{
	return (m_reply->type == REDIS_REPLY_ERROR) ? as_any_string() : std::string_view{};
}

std::string_view Reply::as_any_string() const noexcept
{
	return (m_reply->type == REDIS_REPLY_STRING || m_reply->type == REDIS_REPLY_STATUS || m_reply->type == REDIS_REPLY_ERROR) ?
		std::string_view{m_reply->str, static_cast<std::size_t>(m_reply->len)} : std::string_view{};
}

Reply Reply::as_array(std::size_t i) const noexcept
{
	return m_reply->type == REDIS_REPLY_ARRAY && i < m_reply->elements ?
		Reply{m_reply->element[i]} : Reply{};
}

Reply Reply::as_array(std::size_t i, std::error_code& ec) const noexcept
{
	// If there is already an error, do nothing and because we can't report error.
	if (ec)
		return Reply{};

	if (m_reply->type == REDIS_REPLY_ARRAY && i < m_reply->elements)
	{
		return Reply{m_reply->element[i]};
	}
	else
	{
		ec = Error::field_not_found;
		return Reply{};
	}
}

Reply::iterator Reply::begin() const
{
	return iterator{&m_reply->element[0]};
}

Reply::iterator Reply::end() const
{
	return iterator{&m_reply->element[m_reply->elements]};
}

Reply Reply::operator[](std::size_t i) const noexcept
{
	return as_array(i);
}

std::size_t Reply::array_size() const noexcept
{
	return m_reply->type == REDIS_REPLY_ARRAY ? m_reply->elements : 0ULL;
}

long Reply::as_int() const noexcept
{
	return m_reply->type == REDIS_REPLY_INTEGER ? m_reply->integer : 0;
}

Reply::operator bool() const noexcept
{
	return m_reply->type != REDIS_REPLY_ERROR;
}

long Reply::to_int() const noexcept
{
	auto s = as_any_string();
	return s.empty() ? 0 : std::stol(std::string{s});
	// auto r = std::from_chars()
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

CommandString::CommandString(CommandString&& other)
{
	swap(other);
}

CommandString::~CommandString()
{
	::redisFreeCommand(m_cmd);
}

CommandString& CommandString::operator=(CommandString&& other)
{
	CommandString tmp{std::move(other)};
	swap(tmp);
	return *this;
}

void CommandString::swap(CommandString& other)
{
	std::swap(m_cmd, other.m_cmd);
	std::swap(m_length, other.m_length);
}

}} // end of namespace
