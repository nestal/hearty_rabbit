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

#include "hrb/Ownership.hh"
#include "crypto/Random.hh"

#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <rapidjson/pointer.h>
#include <rapidjson/document.h>

#include <chrono>
#include <iostream>

using namespace hrb;
using namespace std::chrono_literals;

TEST_CASE("Load 3 images in json", "[normal]")
{
	const auto blobids = insecure_random<std::array<ObjectID, 3>>();

	boost::asio::io_context ioc;
	auto redis = redis::connect(ioc);

	int added = 0;

	for (auto&& blobid : blobids)
		Ownership{"testuser"}.link(*redis, "/", blobid, true, [&added](auto ec)
		{
			REQUIRE(!ec);
			added++;
		});

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(added == blobids.size());

	ioc.restart();

	struct MockBlobDb
	{
		std::string load_meta_json(const ObjectID& blob) const
		{
			return to_quoted_hex(blob);
		}
	};

	bool tested = false;
	Ownership{"testuser"}.serialize(*redis, "/", MockBlobDb{}, [&tested](auto&& json, auto ec)
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

	ioc.restart();

	redis->command("MULTI");
	for (auto&& blobid : blobids)
		Ownership{"testuser"}.link(*redis, "/", blobid, false, [&added](auto ec)
		{
			REQUIRE(!ec);
			added--;
		});


	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(added == 0);
}
/*

TEST_CASE("Scan for all containers from testuser")
{
	boost::asio::io_context ioc;
	auto redis = redis::connect(ioc);

	redis->command("MULTI");
	Owne{"testuser", "/"}.link(*redis, insecure_random<ObjectID>());
	redis->command("EXEC", [](auto&& reply, auto ec)
	{
		REQUIRE(!ec);
		REQUIRE(reply.array_size() == 1);
	});

	std::vector<std::string> dirs;

	bool tested = false;
	Collection::scan(*redis, "testuser", 0, [&tested, &dirs](auto begin, auto end, long cursor, auto ec)
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
*/
