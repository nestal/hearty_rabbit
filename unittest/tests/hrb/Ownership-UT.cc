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

#include "hrb/Ownership.hh"
#include "hrb/Ownership.ipp"
#include "hrb/BlobDatabase.hh"
#include "hrb/UploadFile.hh"
#include "hrb/Permission.hh"
#include "crypto/Random.hh"

#include <rapidjson/document.h>

#include <iostream>

using namespace hrb;
using namespace std::chrono_literals;

TEST_CASE("add blob to Ownership", "[normal]")
{
	auto blobid = insecure_random<ObjectID>();

	boost::asio::io_context ioc;
	auto redis = redis::connect(ioc);

	int tested = 0;

	Ownership subject{"owner"};

	subject.link(*redis, "/", blobid, CollEntry{}, [&tested](std::error_code ec)
	{
		REQUIRE(!ec);
		tested++;
	});

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 1);
	ioc.restart();

	// owner access is allowed
	subject.find(*redis, "/", blobid, [&tested](auto&&, std::error_code ec)
	{
		REQUIRE(!ec);
		tested++;
	});

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 2);
	ioc.restart();

	// anonymous access is not allowed
	subject.find(*redis, "/", blobid, [&tested](auto&& entry, std::error_code ec)
	{
		REQUIRE(!ec);
		REQUIRE(!entry.permission().allow(""));
		tested++;
	});

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 3);
	ioc.restart();

	// set permission to public
	subject.set_permission(*redis, "/", blobid, Permission::public_(), [&tested](std::error_code ec)
	{
		REQUIRE(!ec);
		tested++;
	});

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 4);
	ioc.restart();

	// anonymous access is now allowed
	subject.find(*redis, "/", blobid, [&tested](auto&& entry, std::error_code ec)
	{
		REQUIRE(!ec);
		REQUIRE(entry.permission().allow(""));
		tested++;
	});

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 5);
}

TEST_CASE("Load 3 images in json", "[normal]")
{
	const auto blobids = insecure_random<std::array<ObjectID, 3>>();

	boost::asio::io_context ioc;
	auto redis = redis::connect(ioc);

	int added = 0;

	Ownership subject{"testuser"};

	for (auto&& blobid : blobids)
		subject.link(*redis, "some/collection", blobid, CollEntry{}, [&added](auto ec)
		{
			REQUIRE(!ec);
			added++;
		});

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(added == blobids.size());

	ioc.restart();

	bool tested = false;
	subject.serialize(*redis, "some/collection", [&tested](auto&& json, auto ec)
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

	for (auto&& blobid : blobids)
		subject.unlink(*redis, "some/collection", blobid, [&added](auto ec)
		{
			REQUIRE(!ec);
			added--;
		});

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(added == 0);
}

TEST_CASE("Scan for all containers from testuser")
{
	boost::asio::io_context ioc;
	auto redis = redis::connect(ioc);

	Ownership subject{"testuser"};

	bool added = false;
	subject.link(*redis, "/", insecure_random<ObjectID>(), CollEntry{}, [&added](auto ec)
	{
		REQUIRE(!ec);
		added = true;
	});

	std::vector<std::string> dirs;

	bool tested = false;
	subject.scan_collections(*redis, 0, [&tested, &dirs](auto begin, auto end, long cursor, auto ec)
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
	REQUIRE(added);
	REQUIRE(tested);
	REQUIRE(!dirs.empty());
	INFO("dirs.size() " << dirs.size());
	REQUIRE(std::find(dirs.begin(), dirs.end(), "dir:testuser:/") != dirs.end());
}

TEST_CASE("collection entry", "[normal]")
{
	auto s = CollEntry::create({}, "somepic.jpeg", "image/jpeg");
	CollEntry subject{s};
	INFO("entry JSON = " << subject.json());

	REQUIRE(subject.filename() == "somepic.jpeg");
	REQUIRE(subject.mime() == "image/jpeg");
	REQUIRE(subject.permission().allow("sumsum") == false);
	REQUIRE(subject.raw() == s);

	CollEntry same{subject.raw()};
	REQUIRE(same.filename() == "somepic.jpeg");
	REQUIRE(same.mime() == "image/jpeg");
	REQUIRE(same.permission().allow("yungyung") == false);
	REQUIRE(same.raw() == subject.raw());
}
