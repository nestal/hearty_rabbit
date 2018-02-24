/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 2/23/18.
//

#include <catch.hpp>
#include <iostream>
#include "crypto/Random.hh"

#include "hrb/Container.hh"

using namespace hrb;

TEST_CASE("Container tests", "[normal]")
{
	using namespace std::chrono_literals;

	boost::asio::io_context ioc;
	auto redis = redis::connect(ioc);

	ObjectID testid;
	insecure_random(&testid[0], testid.size());

	int tested = 0;
	Container::add(*redis, "test", testid, [&tested, redis, testid](std::error_code ec)
	{
		REQUIRE(!ec);

		Container::is_member(*redis, "test", testid, [&tested](std::error_code ec, bool added)
		{
			REQUIRE(!ec);
			REQUIRE(added);
			tested++;
		});

		Container::load(*redis, "test", [&tested, testid](std::error_code ec, Container&& container)
		{
			REQUIRE(!ec);
			REQUIRE(container.name() == "test");
			REQUIRE(container.size() > 0);
			REQUIRE(!container.empty());

			REQUIRE(std::find(container.begin(), container.end(), testid) != container.end());

			auto json = container.serialize();
			REQUIRE(json.IsObject());

			tested++;
		});
	});
	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 2);
}
