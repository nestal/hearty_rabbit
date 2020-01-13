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

#include "util/Cookie.hh"

using namespace hrb;

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
