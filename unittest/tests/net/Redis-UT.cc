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
/*	SECTION("test array as map")
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
	}*/
}
