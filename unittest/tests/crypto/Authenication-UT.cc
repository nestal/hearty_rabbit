/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/27/18.
//

#include <catch.hpp>

#include "net/Redis.hh"
#include "util/Error.hh"
#include "crypto/Authenication.hh"
#include "crypto/Random.hh"
#include "crypto/Password.hh"

#include <boost/asio/io_context.hpp>

using namespace hrb;

TEST_CASE("Test random number", "[normal]")
{
	auto rand = secure_random_array<std::uint64_t, 2>();
	REQUIRE_NOTHROW(rand[0] > 0 && rand[1] > 0);
}

TEST_CASE("Test password init", "[normal]")
{
	Password subject{"Hello"};
	REQUIRE(subject.get() == "Hello");
	REQUIRE(subject.size() == 5);
	REQUIRE(!subject.empty());

	// different salt must produce different keys
	auto key1 = subject.derive_key("1", 100);
	auto key2 = subject.derive_key("2", 100);
	REQUIRE(key1 != key2);

	subject.clear();
	REQUIRE(subject.empty());
	REQUIRE(subject.size() == 0);

	REQUIRE_NOTHROW(subject.derive_key("salt", 100));
}

TEST_CASE("Test normal user login", "[normal]")
{
	boost::asio::io_context ioc;
	redis::Database redis{ioc, "localhost", 6379};

	bool tested = false;

	add_user("sumsum", Password{"bearbear"}, redis, [&redis, &tested](std::error_code ec)
	{
		REQUIRE(!ec);

		SECTION("correct user")
		{
			verify_user(
				"sumsum", Password{"bearbear"}, redis, [&redis, &tested](std::error_code ec)
				{
					REQUIRE(!ec);
					tested = true;
					redis.disconnect();
				}
			);
		}
		SECTION("incorrect user")
		{
			verify_user(
				"siuyung", Password{"rabbit"}, redis, [&redis, &tested](std::error_code ec)
				{
					REQUIRE(ec == Error::login_incorrect);
					tested = true;
					redis.disconnect();
				}
			);
		}
	});

	using namespace std::chrono_literals;
	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested);
}