/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 3/3/18.
//

#include <catch2/catch.hpp>

#include "crypto/Random.hh"

#include <random>
#include <chrono>
#include <thread>
#include <future>

using namespace hrb;

TEST_CASE("Test random number", "[normal]")
{
	auto rand = system_random_array<std::uint64_t, 2>();
	REQUIRE_NOTHROW(rand[0] > 0 && rand[1] > 0);

	REQUIRE_NOTHROW(rand = user_random_array<std::uint64_t, 2>());
	REQUIRE_NOTHROW(rand[0] > 0 && rand[1] > 0);
}

TEST_CASE("user_random() generates random numbers", "[normal]")
{
	// Very easy....
	REQUIRE(user_random<int>() != user_random<int>());
	REQUIRE(user_random<int>() != user_random<int>());
	REQUIRE(user_random<int>() != user_random<int>());
	REQUIRE(user_random<int>() != user_random<int>());
	REQUIRE(user_random<int>() != user_random<int>());
}

TEST_CASE("multithreaded calls to user_random() generated different random numbers", "[normal]")
{
	auto fut_arr1 = std::async([]{return user_random<std::array<char, 10>>();});
	auto fut_arr2 = std::async([]{return user_random<std::array<char, 10>>();});

	auto arr1 = fut_arr1.get();
	auto arr2 = fut_arr2.get();

	std::transform(arr1.begin(), arr1.end(), arr2.begin(), arr1.begin(), [](auto v1, auto v2)
	{
		REQUIRE(v1 != v2);
		return v1;
	});
}
