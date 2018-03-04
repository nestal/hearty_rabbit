/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 3/4/18.
//

#include <catch.hpp>

#include "hrb/Container.hh"
#include "crypto/Random.hh"

#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <rapidjson/pointer.h>
#include <rapidjson/document.h>

using namespace hrb;

TEST_CASE("Container entries are JSON", "[normal]")
{
	Entry subject{"abc.txt", insecure_random<ObjectID>(), "text/plain"};

	auto json = subject.JSON();
	INFO(json);

	rapidjson::Document doc;
	doc.Parse(json.data(), json.size());
}

TEST_CASE("Container tests", "[normal]")
{
	auto blobid = insecure_random<ObjectID>();

	using namespace std::chrono_literals;

	boost::asio::io_context ioc;
	auto redis = redis::connect(ioc);

	bool tested = false;
	Container::add(*redis, "testuser", "/", "test.jpg", blobid, "image/jpeg", [&tested](auto ec)
	{
		REQUIRE(!ec);
		tested = true;
	});
	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested);
}
