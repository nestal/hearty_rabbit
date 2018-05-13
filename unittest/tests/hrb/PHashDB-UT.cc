/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 5/13/18.
//

#include <catch.hpp>

#include "crypto/Random.hh"
#include "hrb/PHashDb.hh"
#include "image/PHash.hh"
#include "util/Escape.hh"

#include "net/Redis.hh"

#include <iostream>

using namespace hrb;
using namespace std::chrono_literals;

TEST_CASE("find duplicated image in database", "[normal]")
{
	boost::asio::io_context ioc;
	auto redis = redis::connect(ioc);

	PHashDb subject{*redis};

	auto lena = phash(fs::path{__FILE__}.parent_path().parent_path() / "image" / "lena.png");
	REQUIRE(lena.value() > 0);

	auto lena_blob = insecure_random<ObjectID>();
	auto tested = false;

	subject.add(lena_blob, lena);
	subject.similar(lena_blob, [&tested](auto&& matches, auto&& err)
	{
		for (auto&& m : matches)
			std::cout << "similar to " << to_hex(m.id) << std::endl;

		REQUIRE_FALSE(err);
//		REQUIRE(rank == 0);
		tested = true;
	});

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested);
	ioc.restart();

}
