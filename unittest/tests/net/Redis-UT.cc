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
#include <cassert>
#include <chrono>

using namespace hrb::redis;

TEST_CASE("redis server not started", "[normal]")
{
	boost::asio::io_context ioc;
	Connection redis{ioc, "localhost", 1}; // assume no one listen to this port

	bool tested = false;

	// need to write something otherwise hiredis does not detect error
	redis.command([&tested](auto, auto&& ec)
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
	Connection redis{ioc, "localhost", 6379};

	auto tested = 0;

	redis.command([&tested](auto, auto) {tested++;}, "SET key %d", 100);

	SECTION("test sequencial")
	{
		redis.command(
			[&redis, &tested](auto reply, auto&& ec)
			{
				REQUIRE(!ec);

				// Check if redis executes our commands sequencially
				REQUIRE(tested++ == 1);

				REQUIRE(reply.as_string() == "100");

				redis.command(
					[&redis, &tested](auto reply, auto)
					{
						REQUIRE(tested++ == 2);
						REQUIRE(reply.as_int() == 1);

						redis.disconnect();
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
		redis.command([&redis, &tested](auto reply, auto&& ec)
		{
			REQUIRE(!ec);
			REQUIRE(tested++ == 1);

			redis.command([&redis, &tested](auto reply, auto&& ec)
			{
				REQUIRE(!ec);
				REQUIRE(tested++ == 2);
				auto map = reply.map_array();
				REQUIRE(map["field1"].as_string() == "value1");
				REQUIRE(map["field2"].as_string() == "value2");

				redis.disconnect();
			}, "HGETALL test_hash");

		}, "HSET test_hash field1 value1 field2 value2");

		using namespace std::chrono_literals;
		REQUIRE(ioc.run_for(10s) > 0);
		REQUIRE(tested == 3);
	}
}
