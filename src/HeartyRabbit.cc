/*
	Copyright © 2021 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/11/2021.
//

#include "HeartyRabbit.hh"
#include "net/Redis.hh"

#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/coroutine.hpp>

#include <coroutine>
#include <iostream>

namespace hrb::coro {

using namespace hrb::redis;

class Connection : public std::enable_shared_from_this<Connection>
{
public:
	explicit Connection(boost::asio::ip::tcp::socket socket) : m_socket{std::move(socket)}
	{
	}

	Connection(Connection&&) = delete;
	Connection(const Connection&) = delete;
	~Connection() = default;

	Connection& operator=(Connection&&) = delete;
	Connection& operator=(const Connection&) = delete;

	boost::asio::awaitable<Reply> command(CommandString&& cmd)
	{
		try
		{
			auto buffer = cmd.buffer();
			co_await async_write(m_socket, buffer, boost::asio::use_awaitable);

			ReplyReader reader;

			char data[1024];
			while (true)
			{
				std::size_t n = co_await m_socket.async_read_some(boost::asio::buffer(data), boost::asio::use_awaitable);
				reader.feed(data, n);

				if (auto [reply, result] = reader.get(); result == ReplyReader::Result::ok)
				{
					co_return reply;
				}
			}
		}
		catch (...)
		{
			std::cout << "error" << std::endl;
			abort();
		}
	}
private:
	boost::asio::ip::tcp::socket m_socket;
};

} // end of namespace hrb::coro

int main(int argc, char** argv)
{
	boost::asio::io_context ios;

	boost::asio::ip::tcp::socket sock{boost::asio::make_strand(ios)};
	sock.connect(boost::asio::ip::tcp::endpoint{
		boost::asio::ip::make_address("127.0.0.1"),
		6379
	});
	hrb::coro::Connection conn{std::move(sock)};

	hrb::redis::CommandString cmd{"KEYS *"};

	boost::asio::co_spawn(
		ios, [&conn, cmd=std::move(cmd)]() mutable -> boost::asio::awaitable<void>
		{
			auto reply = co_await conn.command(std::move(cmd));
			std::cout << "reply = " << reply.array_size() << std::endl;
		},
	    boost::asio::detached
	);

	ios.run();
	return -1;
}
