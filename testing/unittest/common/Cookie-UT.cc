/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 6/20/18.
//

#include <catch2/catch.hpp>

#include "common/Cookie.hh"
#include "common/UserID.hh"
#include "crypto/Random.hh"

using namespace hrb;
using namespace std::literals;

TEST_CASE("common cookie round-trip", "[normal]")
{
	Cookie subject;
	subject.add("id", "123");
	REQUIRE(subject.str() == "id=123");

	subject.add("HttpOnly");
	REQUIRE(subject.str() == "id=123; HttpOnly");

	Cookie ret{subject.str()};
	REQUIRE(ret.field("id") == "123");
	REQUIRE(ret.has("HttpOnly"));
	REQUIRE_FALSE(ret.has("HTTPONLY"));
}

TEST_CASE("random round-trip", "[normal]")
{
	// Random round-trip
	auto rand = insecure_random<UserID::SessionID>();
	auto cookie = UserID{rand, "test"}.set_cookie(600s);

	INFO("cookie for random session ID is " << cookie);
	auto session = UserID::parse_cookie(cookie);
	REQUIRE(session.has_value());
	REQUIRE(*session == rand);
}

TEST_CASE("Parsing session cookies", "[normal]")
{
	auto session = UserID::parse_cookie(Cookie{"id=0123456789ABCDEF0123456789ABCDEF; somethingelse; "});
	REQUIRE(session.has_value());
	REQUIRE(*session == UserID::SessionID{0x01,0x23,0x45, 0x67, 0x89,0xAB,0xCD,0xEF,0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF});

	session = UserID::parse_cookie(Cookie{"name=value; id=0123456789ABCDEF0123456789ABCDEF; "});
	REQUIRE(session.has_value());
	REQUIRE(*session == UserID::SessionID{0x01,0x23,0x45, 0x67, 0x89,0xAB,0xCD,0xEF,0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF});

	// some lower case characters
	session = UserID::parse_cookie(Cookie{"name=value; id=0123456789abcDEF0123456789ABCdef; "});
	REQUIRE(session.has_value());
	REQUIRE(*session == UserID::SessionID{0x01,0x23,0x45, 0x67, 0x89,0xAB,0xCD,0xEF,0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF});
}
