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
#include <iostream>

using namespace hrb;
using namespace std::chrono_literals;

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

	boost::asio::io_context ioc;
	auto redis = redis::connect(ioc);

	int tested = 0;
	Container::add(*redis, "testuser", "/", "test.jpg", blobid, "image/jpeg", [&tested, redis, blobid](auto ec)
	{
		REQUIRE(!ec);

		Container::find_entry(*redis, "testuser", "/", "test.jpg", [&tested, blobid](auto&& json, auto ec)
		{
			REQUIRE(!ec);
			++tested;

			INFO(json);
		});

		Container::load(*redis, "testuser", "/", [&tested, blobid](auto&& con, auto ec)
		{
			REQUIRE(!ec);

			auto entry = con.find_entry("test.jpg");
			REQUIRE(entry);

			REQUIRE(entry->filename() == "test.jpg");
			REQUIRE(entry->mime() == "image/jpeg");
			REQUIRE(entry->blob() == blobid);

			++tested;
		});
	});
	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 2);
}

TEST_CASE("Load 3 images in json", "[normal]")
{
	auto blobids = insecure_random<std::array<ObjectID, 3>>();

	boost::asio::io_context ioc;
	auto redis = redis::connect(ioc);

	int count = 0;
	for (auto&& blobid : blobids)
	{
		Container::add(*redis, "testuser", "/", to_hex(blobid) + ".jpg", blobid, "image/jpeg", [&count](auto ec)
		{
			REQUIRE(!ec);
			++count;
		});
	}
	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(count == blobids.size());

	ioc.restart();

	bool tested = false;
	Container::serialize(*redis, "testuser", "/", [&tested](auto&& json, auto ec)
	{
		INFO("serialize() error_code: " << ec << " " << ec.message());
		REQUIRE(!ec);
		INFO("serialize result = " << json);

		// try parse the JSON
		rapidjson::Document doc;
		doc.Parse(json.data(), json.size());

		REQUIRE(!doc.HasParseError());

		tested = true;
	});
	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested);
}

TEST_CASE("Scan for all containers from testuser")
{
	boost::asio::io_context ioc;
	auto redis = redis::connect(ioc);

	std::vector<std::string> dirs;

	bool tested = false;
	Container::scan(*redis, "testuser", 0, [&tested, &dirs](auto begin, auto end, long cursor, auto ec)
	{
		INFO("scan() error: " << ec << " " << ec.message());
		REQUIRE(!ec);

		while (begin != end)
		{
			dirs.emplace_back(begin->as_string());
			begin++;
		};

		tested = true;
		return true;
	});

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested);
	REQUIRE(!dirs.empty());
}
