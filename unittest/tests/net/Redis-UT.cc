/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/13/18.
//

#include <catch.hpp>

#include "net/Redis.hh"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>

#include <cassert>
#include <chrono>
#include <iostream>
#include <deque>

using namespace hrb::redis;

TEST_CASE("redis server not started", "[normal]")
{
	boost::asio::io_context ioc;
	auto redis = connect(ioc, "localhost", 1); // assume no one listen to this port

	bool tested = false;

	// need to write something otherwise hiredis does not detect error
	redis->command([&tested](auto, auto&& ec)
	{
		REQUIRE(ec == Error::io);
		tested = true;
	}, "SET key 100");

	using namespace std::chrono_literals;
	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested);
}

TEST_CASE("simple redis", "[normal]")
{
	boost::asio::io_context ioc;
	auto redis = connect(ioc, "localhost", 6379);

	auto tested = 0;

	redis->command([&tested](auto, auto) {tested++;}, "SET key %d", 100);

	SECTION("test sequencial")
	{
		redis->command(
			[redis, &tested](auto reply, auto&& ec)
			{
				REQUIRE(!ec);

				// Check if redis executes our commands sequencially
				REQUIRE(tested++ == 1);

				REQUIRE(reply.as_string() == "100");

				redis->command(
					[redis, &tested](auto reply, auto)
					{
						REQUIRE(tested++ == 2);
						REQUIRE(reply.as_int() == 1);

						redis->disconnect();
					}, "DEL key"
				);
			}, "GET key"
		);

		using namespace std::chrono_literals;
		REQUIRE(ioc.run_for(10s) > 0);
		REQUIRE(tested == 3);
	}
	SECTION("test array as map")
	{
		redis->command([redis, &tested](auto reply, auto&& ec)
		{
			REQUIRE(!ec);
			REQUIRE(tested++ == 1);

			redis->command([redis, &tested](Reply reply, auto&& ec)
			{
				REQUIRE(!ec);
				REQUIRE(tested++ == 2);

				SECTION("verify map_kv_pair()")
				{
					auto [v1, v2] = reply.map_kv_pair("field1", "field2");
					REQUIRE(v1.as_string() == "value1");
					REQUIRE(v2.as_string() == "value2");
				}
				SECTION("verify as_tuple()")
				{
					std::error_code ec2;
					auto [n1, v1, n2, v2] = reply.as_tuple<4>(ec2);
					REQUIRE(n1.as_string() == "field1");
					REQUIRE(v1.as_string() == "value1");
					REQUIRE(n2.as_string() == "field2");
					REQUIRE(v2.as_string() == "value2");
				}
				SECTION("verify begin/end")
				{
					std::vector<std::string> result;
					for (auto&& v : reply)
						result.push_back(std::string{v.as_string()});

					REQUIRE(result[0] == "field1");
					REQUIRE(result[1] == "value1");
					REQUIRE(result[2] == "field2");
					REQUIRE(result[3] == "value2");
				}

				redis->disconnect();
			}, "HGETALL test_hash");

		}, "HSET test_hash field1 value1 field2 value2");

		using namespace std::chrono_literals;
		REQUIRE(ioc.run_for(10s) > 0);
		REQUIRE(tested == 3);
	}
}

class Connection2
{
public:
	Connection2(boost::asio::io_context& ioc, const boost::asio::ip::tcp::endpoint& remote) :
		m_ioc{ioc}, m_socket{m_ioc}
	{
		m_socket.connect(remote);
	}

	template <typename Completion, typename... Args>
	void command(Completion&& completion, Args... args)
	{
		char *cmd{};
		auto len = ::redisFormatCommand(&cmd, args...);
		m_callbacks.push_back(std::forward<Completion>(completion));

		async_write(m_socket, boost::asio::buffer(cmd, len), [this, cmd](auto ec, std::size_t bytes)
		{
			::redisFreeCommand(cmd);

			if (!ec)
			{
				std::cout << "async reading " << ec << std::endl;
				m_socket.async_read_some(
					boost::asio::buffer(m_read_buf),
					[this](auto ec, auto read){ on_read(ec, read); }
				);
			}
		});
	}

private:
	void on_read(boost::system::error_code ec, std::size_t bytes)
	{
		std::cout << "read " << bytes << " bytes: " << std::string_view{m_read_buf, bytes} << std::endl;
		if (!ec)
		{
			::redisReaderFeed(m_reader, m_read_buf, bytes);

			::redisReply *reply{};
			auto result = ::redisReaderGetReply(m_reader, (void**)&reply);

			if (result == REDIS_OK && reply)
			{
				m_callbacks.front()(hrb::redis::Reply{reply});
				m_callbacks.pop_front();
			}
			else if (result == REDIS_OK)
				m_socket.async_read_some(
					boost::asio::buffer(m_read_buf),
					[this](auto ec, auto read){on_read(ec, read);}
				);
		}
	}

private:
	boost::asio::io_context& m_ioc;
	boost::asio::ip::tcp::socket m_socket;

	char m_read_buf[4096];
	::redisReader *m_reader{::redisReaderCreate()};

	std::deque<std::function<void(hrb::redis::Reply)>> m_callbacks;
};

TEST_CASE("custom redis", "[normal]")
{
	using namespace boost::asio;
	boost::asio::io_context ioc;
	Connection2 conn{ioc, {ip::make_address("127.0.0.1"), 6379}};

	conn.command([](hrb::redis::Reply reply)
	{
		std::cout << "reply = " << reply.as_status() << std::endl;
	}, "SET key 1001");
	ioc.run();
}