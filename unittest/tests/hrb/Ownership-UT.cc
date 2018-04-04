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

#include <iostream>

using namespace hrb;
using namespace std::chrono_literals;

TEST_CASE("list of collection owned by user", "[normal]")
{
	auto blobid = insecure_random<ObjectID>();

	boost::asio::io_context ioc;
	auto redis = redis::connect(ioc);

	Ownership subject{"owner"};

	int tested = 0;
	subject.link(*redis, "/", blobid, CollEntry{}, [&tested](std::error_code ec)
	{
		REQUIRE(!ec);
		tested++;
	});

	// assert that the collection is added
	subject.scan_all_collections(*redis, "owner", [&tested](auto&& json, auto ec)
	{
		REQUIRE(!ec);
		REQUIRE(json.find("colls") != json.end());

		std::vector<std::string> colls;
		for (auto&& it : json["colls"].items())
			colls.emplace_back(it.key());

		REQUIRE(std::find(colls.begin(), colls.end(), "/") != colls.end());
		tested++;
	});

	// assert the blob backlink points back to the collection
	std::vector<std::string> refs;
	subject.find_reference(*redis, blobid, [&refs](auto coll)
	{
		refs.emplace_back(coll);
	});

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 2);
	REQUIRE(std::find(refs.begin(), refs.end(), "/") != refs.end());
	ioc.restart();

	// remove all blobs in the collection
	subject.serialize(*redis, "owner", "/", [&tested, redis](auto&& json, auto ec)
	{
		INFO("serialize() return " << json);
		auto jdoc = nlohmann::json::parse(json);

		for (auto&& blob : jdoc["elements"].items())
		{
			INFO("blob = " << blob.key());
			Ownership{"owner"}.unlink(*redis, "/", *hex_to_object_id(blob.key()), [](auto&& ec)
			{
				REQUIRE(!ec);
			});
		}

		tested++;
	});

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 3);
	ioc.restart();

	// assert that the collection "/" does not exist anymore, because all its blobs are removed
	subject.scan_all_collections(*redis, "owner", [&tested](auto&& json, auto ec)
	{
		REQUIRE(!ec);
		REQUIRE(json.find("colls") != json.end());

		std::vector<std::string> colls;
		for (auto&& it : json["colls"].items())
			REQUIRE(it.key() != std::string{"/"});
		tested++;
	});
	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 4);
}

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

	// verify that the newly added blob is in the public list
	bool found = false;
	Ownership::list_public_blobs(*redis, [&found, blobid](auto blob)
	{
		if (blob == blobid)
			found = true;
	});

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(found);
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
	ioc.restart();

	// move to another new collection
	subject.move_blob(*redis, "/", "someother", blobid, [&tested](std::error_code ec)
	{
		REQUIRE(!ec);
		tested++;
	});

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 6);
	ioc.restart();

	// check if it is in the new collection
	subject.find(*redis, "someother", blobid, [&tested](auto&& entry, std::error_code ec)
	{
		REQUIRE(!ec);
		REQUIRE(entry.permission().allow(""));
		tested++;
	});

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 7);
}

TEST_CASE("Load 3 images in json", "[normal]")
{
	const auto blobids = insecure_random<std::array<ObjectID, 3>>();

	boost::asio::io_context ioc;
	auto redis = redis::connect(ioc);

	int added = 0;

	Ownership subject{"testuser"};

	for (auto&& blobid : blobids)
	{
		auto s = CollEntry::create(Permission::private_(), "file.jpg", "image/jpeg");
		subject.link(
			*redis, "some/collection", blobid, CollEntry{s}, [&added](auto ec)
			{
				REQUIRE(!ec);
				added++;
			}
		);
	}

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(added == blobids.size());

	ioc.restart();

	bool tested = false;
	subject.serialize(*redis, "testuser", "some/collection", [&tested, &blobids](auto&& jstr, auto ec)
	{
		INFO("serialize() error_code: " << ec << " " << ec.message());
		INFO("serialize result = " << jstr);

		REQUIRE(!ec);

		// try parse the JSON
		using json = nlohmann::json;
		std::cout << "jstr = " << jstr << std::endl;
		auto doc = json::parse(jstr);
		std::cout << "after jstr = " << jstr << std::endl;

		REQUIRE(!doc.empty());
		REQUIRE(
			doc.value(json::json_pointer{"/owner"}, "") == "testuser"
		);
		REQUIRE(
			doc.value(json::json_pointer{"/username"}, "") == "testuser"
		);
		REQUIRE(
			doc.value(json::json_pointer{"/collection"}, "") == "some/collection"
		);

		for (auto&& blobid : blobids)
		{
			REQUIRE(
				doc.value(json::json_pointer{"/elements/" + to_hex(blobid) + "/perm"}, "") == "private"
			);
			REQUIRE(
				doc.value(json::json_pointer{"/elements/" + to_hex(blobid) + "/filename"}, "") == "file.jpg"
			);
			REQUIRE(
				doc.value(json::json_pointer{"/elements/" + to_hex(blobid) + "/mime"}, "") == "image/jpeg"
			);
		}

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

TEST_CASE("Query blob of testuser")
{
	boost::asio::io_context ioc;
	auto redis = redis::connect(ioc);

	Ownership subject{"testuser"};

	auto blobid = insecure_random<ObjectID>();

	auto ce_str = CollEntry::create(Permission::public_(), "haha.jpeg", "image/jpeg");

	int tested = 0;
	subject.link(
		*redis, "somecoll", blobid, CollEntry{ce_str}, [&tested](auto ec)
		{
			REQUIRE(!ec);
			tested++;
		}
	);
	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 1);
	ioc.restart();

	subject.query_blob(*redis, blobid, [&tested](auto&& user, auto&& coll, auto&& entry)
	{
		// There should be only one collection that owns the blob
		REQUIRE(user == "testuser");
		REQUIRE(coll == "somecoll");
		REQUIRE(entry.permission() == Permission::public_());
		REQUIRE(entry.filename() == "haha.jpeg");
		REQUIRE(entry.mime() == "image/jpeg");
		tested++;
	});

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 2);
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
	subject.scan_all_collections(*redis, "",
		[&dirs, &tested](auto&& jdoc, auto ec)
		{
			INFO("scan() error: " << ec << " " << ec.message());
			REQUIRE(!ec);
			tested = true;

			REQUIRE(jdoc["username"] == "");
			REQUIRE(jdoc["owner"]    == "testuser");

			for (auto&& it : jdoc["colls"].items())
				dirs.push_back(it.key());
		}
	);

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(added);
	REQUIRE(tested);
	REQUIRE(!dirs.empty());
	INFO("dirs.size() " << dirs.size());
	REQUIRE(std::find(dirs.begin(), dirs.end(), "/") != dirs.end());
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

TEST_CASE("Collection ctor", "[normal]")
{
	Ownership::Collection subject{"dir:user:path"};
	REQUIRE(subject.user() == "user");
	REQUIRE(subject.path() == "path");
	REQUIRE(subject.redis_key() == "dir:user:path");

	Ownership::Collection path_with_colon{"dir:sumsum::path::"};
	REQUIRE(path_with_colon.user() == "sumsum");
	REQUIRE(path_with_colon.path() == ":path::");
	REQUIRE(path_with_colon.redis_key() == "dir:sumsum::path::");

	Ownership::Collection path_with_slash{"dir:siuyung:/some/collection:path"};
	REQUIRE(path_with_slash.user() == "siuyung");
	REQUIRE(path_with_slash.path() == "/some/collection:path");
	REQUIRE(path_with_slash.redis_key() == "dir:siuyung:/some/collection:path");
}
