/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 3/3/18.
//

#include <catch.hpp>

#include "crypto/Random.hh"

#include <random>
#include <chrono>
#include <thread>
#include <future>

using namespace hrb;

TEST_CASE("Test random number", "[normal]")
{
	auto rand = secure_random_array<std::uint64_t, 2>();
	REQUIRE_NOTHROW(rand[0] > 0 && rand[1] > 0);

	REQUIRE_NOTHROW(rand = insecure_random_array<std::uint64_t, 2>());
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
		secure_random<std::uint64_t>();
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

TEST_CASE("insecure_random() generates random numbers", "[normal]")
{
	// Very easy....
	REQUIRE(insecure_random<int>() != insecure_random<int>());
	REQUIRE(insecure_random<int>() != insecure_random<int>());
	REQUIRE(insecure_random<int>() != insecure_random<int>());
	REQUIRE(insecure_random<int>() != insecure_random<int>());
	REQUIRE(insecure_random<int>() != insecure_random<int>());
}

TEST_CASE("copy and move ctor duplicate Blake2x random stream", "[normal]")
{
	std::array<Blake2x::result_type, 20> original, copy, move;

	Blake2x g;
	auto gcopy{g};
	auto gmove{std::move(g)};

	std::generate(original.begin(), original.end(), std::ref(g));
	std::generate(copy.begin(), copy.end(), std::ref(gcopy));
	std::generate(move.begin(), move.end(), std::ref(gmove));

	REQUIRE(std::equal(original.begin(), original.end(), copy.begin()));
	REQUIRE(std::equal(original.begin(), original.end(), move.begin()));
}

TEST_CASE("multithreaded calls to insecure_random() generated different random numbers", "[normal]")
{
	auto fut_arr1 = std::async([]{return insecure_random<std::array<Blake2x::result_type, 10>>();});
	auto fut_arr2 = std::async([]{return insecure_random<std::array<Blake2x::result_type, 10>>();});

	auto arr1 = fut_arr1.get();
	auto arr2 = fut_arr2.get();

	std::transform(arr1.begin(), arr1.end(), arr2.begin(), arr1.begin(), [](auto v1, auto v2)
	{
		REQUIRE(v1 != v2);
		return v1;
	});
}
