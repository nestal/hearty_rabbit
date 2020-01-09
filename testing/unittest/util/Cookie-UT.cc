/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 6/5/18.
//

#include <catch2/catch.hpp>

#include "util/Cookie.hh"

using namespace hrb;

TEST_CASE("parse expire time from cookie", "[normal]")
{
	Cookie subject{" id=a3fWa; Expires=Wed, 21 Oct 2015 07:28:00 GMT; Secure; HttpOnly"};
	REQUIRE(subject.has("Expires"));
	REQUIRE(subject.has("id"));
	REQUIRE(subject.expires() < std::chrono::system_clock::now());

	auto tt = std::chrono::system_clock::to_time_t(subject.expires());
	auto tm = std::gmtime(&tt);
	REQUIRE(tm->tm_hour == 7);
	REQUIRE(tm->tm_min == 28);
	REQUIRE(tm->tm_sec == 0);
	REQUIRE(tm->tm_year == 115);
	REQUIRE(tm->tm_mon == 9);
	REQUIRE(tm->tm_mday == 21);

	REQUIRE(subject.field("id") == "a3fWa");
}

TEST_CASE("cookie parsing round-trip", "[normal]")
{
	Cookie subject;
	subject.add("sid", "some ID");
	REQUIRE_FALSE(subject.has("id"));
	REQUIRE(subject.has("sid"));
	REQUIRE(subject.field("sid") == "some ID");
}
