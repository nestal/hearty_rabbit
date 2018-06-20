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
#include "common/Cookie.hh"
#include "common/Error.hh"
#include "crypto/Authentication.hh"
#include "crypto/Authentication.ipp"
#include "crypto/Password.hh"
#include "crypto/Random.hh"

#include <boost/asio/io_context.hpp>
#include <boost/algorithm/hex.hpp>

using namespace hrb;

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

TEST_CASE("Default Authenication ctor construct invalid authenication", "[normal]")
{
	REQUIRE(!Authentication{}.valid());
}

TEST_CASE("Test normal user login", "[normal]")
{
	using namespace std::chrono_literals;
	boost::asio::io_context ioc;
	auto redis = redis::connect(ioc);

	bool tested = false;

	// for system test
	Authentication::add_user("siuyung", Password{"rabbit"}, *redis, [](auto&&) {});

	Authentication::add_user("sumsum",  Password{"bearbear"}, *redis, [redis, &tested](std::error_code ec)
	{
		INFO("add_user() result = " << ec.message());
		REQUIRE(!ec);

		SECTION("correct user")
		{
			// Verify user with a username in a different case.
			// Since username is case-insensitive, it should still work.
			Authentication::verify_user(
				"suMSum", Password{"bearbear"}, *redis, 60s, [redis, &tested](std::error_code ec, auto&& session)
				{
					INFO("verify_user(correct) result = " << ec.message());
					REQUIRE(!ec);
					REQUIRE(session.valid());

					Authentication::verify_session(
						session.session(), *redis, 60s,
						[&tested](std::error_code ec, auto&& auth)
						{
							// Username returned is always lower case.
							REQUIRE(!ec);
							REQUIRE(auth.valid());
							REQUIRE_FALSE(auth.is_guest());
							REQUIRE(auth.username() == "sumsum");
							tested = true;
						}
					);
				}
			);
		}
		SECTION("incorrect password")
		{
			using namespace std::chrono_literals;
			Authentication::verify_user(
				"siuyung", Password{"not rabbit"}, *redis, 60s, [&tested](std::error_code ec, auto&& session)
				{
					INFO("verify_user(incorrect) result = " << ec.message());
					REQUIRE(ec == Error::login_incorrect);
					REQUIRE(!session.valid());
					REQUIRE_FALSE(session.is_guest());
					tested = true;
				}
			);
		}
		SECTION("verify random session ID")
		{
			auto cookie = insecure_random<UserID::SessionID>();

			Authentication::verify_session(cookie, *redis, 60s, [&tested](std::error_code ec, auto&& session)
				{
					INFO("verify_session(incorrect) result = " << ec.message());
					REQUIRE(!ec);
					REQUIRE_FALSE(session.valid());
					REQUIRE_FALSE(session.is_guest());
					tested = true;
				}
			);
		}
	});

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested);
	ioc.restart();

	auto tested_count = 0;

	// It is easier to simulate session expiry in UT by changing session length
	Authentication::verify_user(
		"sumsum", Password{"bearbear"}, *redis, 60s, [redis, &tested_count](std::error_code ec, auto&& session)
		{
			// Force session renew by setting the session length (180s) required to be more than
			// twice of the value specified when created (60s)
			Authentication::verify_session(
				session.session(), *redis, 180s,
				[&tested_count, old_cookie = session.session()](std::error_code ec, auto&& auth)
				{
					REQUIRE(!ec);
					REQUIRE(auth.valid());
					REQUIRE_FALSE(auth.is_guest());
					REQUIRE(auth.username() == "sumsum");

					// Verify cookie changed
					REQUIRE(auth.session() != old_cookie);
					tested_count++;
				}
			);

			// Session will not be renewed when verified again, but verification will still
			// succeed.
			Authentication::verify_session(
				session.session(), *redis, 180s,
				[&tested_count, old_cookie = session.session()](std::error_code ec, auto&& auth)
				{
					REQUIRE(!ec);
					REQUIRE(auth.valid());
					REQUIRE(auth.username() == "sumsum");
					REQUIRE(auth.session() == old_cookie);
					tested_count++;
				}
			);
		}
	);
	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested_count == 2);
}

TEST_CASE("Sharing resource to guest", "[normal]")
{
	using namespace std::chrono_literals;
	boost::asio::io_context ioc;
	auto redis = redis::connect(ioc);

	auto tested = 0;
	Authentication::share_resource("sumsum", "dir:", 3600s, *redis, [&tested, redis](auto&& auth, auto ec)
	{
		REQUIRE(!ec);
		REQUIRE(auth.id().is_guest());
		REQUIRE(auth.valid());

		tested++;

		auth.is_shared_resource("dir:", *redis, [&tested](bool shared, auto ec)
		{
			REQUIRE_FALSE(ec);
			REQUIRE(shared);

			tested++;
		});

		Authentication::list_guests("sumsum", "dir:", *redis, [&tested, auth](auto&& guests, auto ec)
		{
			REQUIRE_FALSE(ec);

			for (auto&& guest : guests)
				if (guest == auth)
				{
					tested++;
					break;
				}
		});
	});

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 3);
	ioc.restart();

	bool found = false;

	Authentication someone_else{insecure_random<UserID::SessionID>(), "someone"};
	someone_else.is_shared_resource("dir:", *redis, [&found](bool shared, auto ec)
	{
		REQUIRE_FALSE(shared);
		found = false;
	});

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(true);
	ioc.restart();
}
