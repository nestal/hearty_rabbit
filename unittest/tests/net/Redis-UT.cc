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
#include <util/Error.hh>

using namespace hrb::redis;

TEST_CASE("redis server not started", "[normal]")
{
	boost::asio::io_context ioc;
	REQUIRE_THROWS(connect(ioc, {boost::asio::ip::make_address("127.0.0.1"), 1})); // assume no one listen to this port
}

TEST_CASE("simple redis", "[normal]")
{
	boost::asio::io_context ioc;
	auto redis = connect(ioc);

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

TEST_CASE("redis reply reader simple normal cases", "[normal]")
{
	ReplyReader subject;

	SECTION("normal one pass")
	{
		std::string_view str{"+OK\r\n"};
		subject.feed(str.data(), str.size());
		auto[reply, result] = subject.get();
		REQUIRE(result == ReplyReader::Result::ok);
		REQUIRE(reply.as_status() == "OK");
	}
	SECTION("two replies in one pass")
	{
		// See https://redis.io/topics/protocol for detail string format
		std::string_view str{"$3\r\nfoo\r\n$3\r\nbar\r\n"};
		subject.feed(str.data(), str.size());

		auto[reply, result] = subject.get();
		REQUIRE(result == ReplyReader::Result::ok);
		REQUIRE(reply.as_string() == "foo");

		std::tie(reply, result) = subject.get();
		REQUIRE(result == ReplyReader::Result::ok);
		REQUIRE(reply.as_string() == "bar");
	}
	SECTION("one reply in two passes")
	{
		std::string_view str{"$10\r\n0123456789\r\n"};
		REQUIRE(str.size() > 5);

		subject.feed(str.data(),   5);
		auto[reply, result] = subject.get();
		REQUIRE(result == ReplyReader::Result::not_ready);
		REQUIRE(!reply);
		REQUIRE(reply.as_string().empty());

		subject.feed(str.data()+5, str.size()-5);
		std::tie(reply, result) = subject.get();
		REQUIRE(result == ReplyReader::Result::ok);
		REQUIRE(reply.as_string() == "0123456789");

		std::tie(reply, result) = subject.get();
		REQUIRE(result == ReplyReader::Result::not_ready);
		REQUIRE(!reply);
		REQUIRE(reply.as_string().empty());
	}
	SECTION("two reples in two passes, interleaves")
	{
		using namespace std::literals;

		// string_view(const char*) can't be used to initialize a string with
		// null character. Need to pass sizeof() whole buffer as string length.
		char raw_str[] = "$10\r\n0123\0__789\r\n$15\r\nabcde0\r\n3456789\r\n";
		std::string_view str{raw_str, sizeof(raw_str)-1};

		REQUIRE(str.size() > 7);

		subject.feed(str.data(), 7);
		str.remove_prefix(7);

		auto[reply, result] = subject.get();
		REQUIRE(result == ReplyReader::Result::not_ready);
		REQUIRE(!reply);

		REQUIRE(str.size() >= 10);
		subject.feed(str.data(), 10);
		str.remove_prefix(10);

		std::tie(reply, result) = subject.get();
		REQUIRE(result == ReplyReader::Result::ok);
		REQUIRE(reply.as_string() == "0123\0__789"sv);

		subject.feed(str.data(), str.size());

		std::tie(reply, result) = subject.get();
		REQUIRE(result == ReplyReader::Result::ok);
		REQUIRE(reply.as_string() == "abcde0\r\n3456789"sv);
	}
}

TEST_CASE("redis reply reader simple error cases", "[normal]")
{
	ReplyReader subject;

	SECTION("empty string")
	{
		subject.feed("", 0);
		auto[reply, result] = subject.get();
		REQUIRE(result == ReplyReader::Result::not_ready);
		REQUIRE(!reply);
		REQUIRE(reply.as_string().empty());
	}
}

TEST_CASE("transaction", "[normal]")
{
	boost::asio::io_context ioc;
	auto redis = connect(ioc);

	int tested = 0;
	auto expect_success = [&tested](auto, auto ec)
	{
		REQUIRE(!ec);
		tested++;
	};

	SECTION("execute transaction")
	{
		redis->command(expect_success, "MULTI");
		redis->command(expect_success, "SET in_transaction 100");

		redis->command(expect_success, "EXEC");
	}
	SECTION("discard transaction")
	{
		auto expect_abort = [&tested](auto, auto ec)
		{
			REQUIRE(ec == hrb::Error::redis_transaction_aborted);
			tested++;
		};

		redis->command(expect_success, "MULTI");
		redis->command(expect_abort, "SET in_transaction 100");

		redis->command(expect_success, "DISCARD");
	}

	using namespace std::chrono_literals;
	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 3);
}

TEST_CASE("std::function() as callback", "[normal]")
{
	bool tested = false;
	std::function<void(Reply, std::error_code)> expect_success{[&tested](Reply, std::error_code ec)
	{
		REQUIRE(!ec);
		tested = true;
	}};

	boost::asio::io_context ioc;
	auto redis = connect(ioc);

	redis->command(expect_success, "SET func_callback 120");

	using namespace std::chrono_literals;
	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested);
}
