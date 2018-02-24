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
#include "crypto/Authentication.hh"
#include "crypto/Random.hh"
#include "crypto/Password.hh"

#include <boost/asio/io_context.hpp>
#include <boost/algorithm/hex.hpp>

#include <random>

using namespace hrb;

TEST_CASE("Test random number", "[normal]")
{
	auto rand = secure_random_array<std::uint64_t, 2>();
	REQUIRE_NOTHROW(rand[0] > 0 && rand[1] > 0);
}

TEST_CASE("Blake2x is faster than urandom", "[normal]")
{
	Blake2x g;

	const auto trial = 100000;
	using namespace std::chrono;

	auto blake_start = system_clock::now();
	for (auto i = 0 ; i < trial; i++)
		g();
	auto blake_elapse = system_clock::now() - blake_start;

	auto urand_start = system_clock::now();
	for (auto i = 0 ; i < trial; i++)
		secure_random<Blake2x::result_type>();
	auto urand_elapse = system_clock::now() - urand_start;

	INFO("blake2x x " << trial << ": " << duration_cast<milliseconds>(blake_elapse).count() << "ms");
	INFO("urandom x " << trial << ": " << duration_cast<milliseconds>(urand_elapse).count() << "ms");
	REQUIRE(blake_elapse < urand_elapse);

	std::mt19937_64 mt{g()};
	auto mt_start = system_clock::now();
	for (auto i = 0 ; i < trial; i++)
		mt();
	auto mt_elapse = system_clock::now() - mt_start;

	INFO("mt19937 x " << trial << ": " << duration_cast<milliseconds>(mt_elapse).count() << "ms");
	INFO("blake2x x " << trial << ": " << duration_cast<milliseconds>(blake_elapse).count() << "ms");
	INFO("urandom x " << trial << ": " << duration_cast<milliseconds>(urand_elapse).count() << "ms");
	REQUIRE(mt_elapse < blake_elapse);
}

TEST_CASE("Blake2x generates random numbers", "[normal]")
{
	// Very easy....
	Blake2x g;
	REQUIRE(g() != g());
	REQUIRE(g() != g());
	REQUIRE(g() != g());
	REQUIRE(g() != g());
	REQUIRE(g() != g());
}

TEST_CASE("Test password init", "[normal]")
{
	Password subject{"Hello"};
	REQUIRE(subject.get() == "Hello");
	REQUIRE(subject.size() == 5);
	REQUIRE(!subject.empty());

	// different salt must produce different keys
	auto key1 = subject.derive_key("1", 100, "sha512");
	auto key2 = subject.derive_key("2", 100, "sha512");
	REQUIRE(key1 != key2);

	subject.clear();
	REQUIRE(subject.empty());
	REQUIRE(subject.size() == 0);

	REQUIRE_NOTHROW(subject.derive_key("salt", 100, "sha512" ));
}

TEST_CASE("Test normal user login", "[normal]")
{
	boost::asio::io_context ioc;
	auto redis = redis::connect(ioc);

	bool tested = false;

	Authentication::add_user("sumsum", Password{"bearbear"}, *redis, [redis, &tested](std::error_code ec)
	{
		INFO("add_user() result = " << ec.message());
		REQUIRE(!ec);

		SECTION("correct user")
		{
			// Verify user with a username in a different case.
			// Since username is case-insensitive, it should still work.
			Authentication::verify_user(
				"suMSum", Password{"bearbear"}, *redis, [redis, &tested](std::error_code ec, auto&& session)
				{
					INFO("verify_user(correct) result = " << ec.message());
					REQUIRE(!ec);
					REQUIRE(session.valid());

					Authentication::verify_session(session.cookie(), *redis, [redis, &tested](std::error_code ec, auto&& auth)
					{
						// Username returned is always lower case.
						REQUIRE(!ec);
						REQUIRE(auth.valid());
						REQUIRE(auth.user() == "sumsum");
						redis->disconnect();
						tested = true;
					});
				}
			);
		}
		SECTION("incorrect user")
		{
			Authentication::verify_user(
				"siuyung", Password{"rabbit"}, *redis, [redis, &tested](std::error_code ec, auto&& session)
				{
					INFO("verify_user(incorrect) result = " << ec.message());
					REQUIRE(ec == Error::login_incorrect);
					REQUIRE(!session.valid());
					tested = true;
					redis->disconnect();
				}
			);
		}
	});

	using namespace std::chrono_literals;
	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested);
}

TEST_CASE("Parsing cookie", "[normal]")
{
	auto session = parse_cookie("id=0123456789ABCDEF0123456789ABCDEF; somethingelse; ");
	REQUIRE(session.has_value());
	REQUIRE(*session == Authentication::Cookie{0x01,0x23,0x45, 0x67, 0x89,0xAB,0xCD,0xEF,0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF});

	session = parse_cookie("name=value; id=0123456789ABCDEF0123456789ABCDEF; ");
	REQUIRE(session.has_value());
	REQUIRE(*session == Authentication::Cookie{0x01,0x23,0x45, 0x67, 0x89,0xAB,0xCD,0xEF,0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF});

	// some lower case characters
	session = parse_cookie("name=value; id=0123456789abcDEF0123456789ABCdef; ");
	REQUIRE(session.has_value());
	REQUIRE(*session == Authentication::Cookie{0x01,0x23,0x45, 0x67, 0x89,0xAB,0xCD,0xEF,0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF});

	// Random round-trip
	std::mt19937_64 salt_generator{secure_random<std::uint64_t>()};
	Authentication::Cookie rand{};
	std::generate(rand.begin(), rand.end(), std::ref(salt_generator));
	auto cookie = Authentication{rand, "test"}.set_cookie();
	INFO("cookie for random session ID is " << cookie);
	session = parse_cookie(cookie);
	REQUIRE(session.has_value());
	REQUIRE(*session == rand);
}
