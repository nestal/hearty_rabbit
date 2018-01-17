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
#include <iostream>

TEST_CASE("simple redis", "[normal]")
{
	boost::asio::io_context ic;
	hrb::Database redis{ic, "localhost", 6379};

	redis.command([](auto) {}, "SET key %d", 100);
	redis.command([&redis](auto reply)
	{
	    REQUIRE(reply);
		REQUIRE(reply->type == REDIS_REPLY_STRING);

		std::string_view reply_str{reply->str, static_cast<unsigned>(reply->len)};
		REQUIRE(reply_str == "100");

		redis.command([&redis](auto reply)
		{
		    REQUIRE(reply);
			REQUIRE(reply->type == REDIS_REPLY_INTEGER);
			REQUIRE(reply->integer == 1);

			redis.disconnect();
		}, "DEL key");
	}, "GET key");

	ic.run();
}
