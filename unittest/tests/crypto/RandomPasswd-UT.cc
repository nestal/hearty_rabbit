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

#include "crypto/Random.hh"
#include "crypto/Password.hh"

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
