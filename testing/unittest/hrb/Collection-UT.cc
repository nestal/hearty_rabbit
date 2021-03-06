/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 6/7/18.
//

#include "hrb/Blob.hh"
#include "hrb/Collection.hh"
#include "hrb/CollectionList.hh"
#include "crypto/Random.hh"

#include <catch2/catch.hpp>

using namespace hrb;
using namespace std::chrono_literals;

TEST_CASE("simple BlobInode <-> JSON round-trip", "[normal]")
{
	BlobInode subject{Permission::public_(), "abc.txt", "text/plain", Timestamp{101s}};

	nlohmann::json json(subject);
	INFO("Subject JSON: " << json);

	auto ret = json.get<BlobInode>();
	REQUIRE(subject.mime == ret.mime);
	REQUIRE(subject.filename == ret.filename);
	REQUIRE(subject.perm == ret.perm);
	REQUIRE(subject.timestamp == ret.timestamp);
}

TEST_CASE("simple Collection <-> JSON round-trip", "[normal]")
{
	auto cover = insecure_random<ObjectID>();

	auto abc = insecure_random<ObjectID>();
	auto img = insecure_random<ObjectID>();

	Collection subject{"some_coll", "sumyung", nlohmann::json::object({{"cover", cover}})};
	subject.add_blob(abc, {Permission::public_(), "abc.txt", "text/plain", Timestamp{101s}});
	subject.add_blob(img, {Permission::private_(), "image.jpeg", "image/jpeg", Timestamp{1h}});

	// try updating one blob's timestamp
	subject.update_timestamp(abc, Timestamp{342s});

	// don't use {} to construct nlohmann. it will produce an JSON array
	nlohmann::json json(subject);
	INFO("subject JSON: " << json);

	REQUIRE(subject.cover().has_value());
	REQUIRE(subject.cover() == cover);

	auto ret = json.get<Collection>();
	REQUIRE(subject.owner() == ret.owner());
	REQUIRE(subject.name() == ret.name());
	REQUIRE(subject.cover() == ret.cover());
	REQUIRE(ret.get_blob(abc).has_value());
	REQUIRE(ret.get_blob(abc)->info().timestamp == Timestamp{342s});
	REQUIRE(ret.get_blob(img)->info().timestamp == Timestamp{1h});

	ret.remove_blob(abc);
	REQUIRE(subject != ret);
}

// verify operator==() before using it to verify result of other tests
TEST_CASE("CollectionList operator==()", "[normal]")
{
	CollectionList subject;
	subject.add("sumsum", "default", insecure_random<ObjectID>());
	subject.add("sumsum", "some coll", insecure_random<ObjectID>());

	CollectionList subject2;
	subject2.add("sumsum", "default", insecure_random<ObjectID>());
	subject2.add("sumsum", "some coll", insecure_random<ObjectID>());

	REQUIRE_FALSE(subject == subject2);
	REQUIRE(subject != subject2);

	CollectionList subject3;
	subject3.add("sumsum", "default", *subject.at(0).cover());
	subject3.add("sumsum", "some coll", *subject.at(1).cover());

	REQUIRE(subject == subject3);
	REQUIRE_FALSE(subject != subject3);
}

TEST_CASE("simple CollectionList <-> JSON round-trip", "[normal]")
{
	CollectionList subject;
	subject.add("sumsum", "default", insecure_random<ObjectID>());
	subject.add("sumsum", "some coll", insecure_random<ObjectID>());
	REQUIRE(subject.size() == 2);

	// additional property besides cover
	subject.at(1).meta().emplace("field", "value");

	// don't use {} to construct nlohmann. it will produce an JSON array
	nlohmann::json json(subject);
	INFO("subject JSON: " << json);

	auto ret = json.get<CollectionList>();
	REQUIRE(ret.size() == 2);
	REQUIRE(subject == ret);
	REQUIRE(ret.at(1).meta()["field"] == "value");
}

TEST_CASE("simple BlobList <-> JSON round-trip", "[normal]")
{
	BlobElements subject;
	subject.emplace_back("sumsum", "coll", insecure_random<ObjectID>(), BlobInode{Permission::shared(), "abc.txt", "text/css"});

	REQUIRE(subject.size() == 1);
	for (auto&& e : subject)
	{
		REQUIRE(e.owner() == "sumsum");
		REQUIRE(e.collection() == "coll");
		REQUIRE(e.info().filename == "abc.txt");
		REQUIRE(e.info().mime == "text/css");
		REQUIRE(e.info().perm == Permission::shared());
	}
	subject.emplace_back("yung", "cool", insecure_random<ObjectID>(), BlobInode{Permission::private_(), "IMG_0102.JPG", "image/jpeg"});
	REQUIRE(subject.size() == 2);

	nlohmann::json json(std::move(subject));

	auto entries = json.get<BlobElements>();
	REQUIRE(entries.size() == 2);

	// sort it by owner name for easy checking
	std::sort(entries.begin(), entries.end(), [](auto&& e1, auto&& e2){return e1.owner() < e2.owner();});
	REQUIRE(entries.size() == 2);

	REQUIRE(entries[0].owner() == "sumsum");
	REQUIRE(entries[0].collection() == "coll");
	REQUIRE(entries[0].info().filename == "abc.txt");
	REQUIRE(entries[0].info().mime == "text/css");
	REQUIRE(entries[0].info().perm == Permission::shared());
	REQUIRE(entries[1].owner() == "yung");
	REQUIRE(entries[1].collection() == "cool");
	REQUIRE(entries[1].info().filename == "IMG_0102.JPG");
	REQUIRE(entries[1].info().mime == "image/jpeg");
	REQUIRE(entries[1].info().perm == Permission::private_());
}

TEST_CASE("empty BlobList serialization", "[normal]")
{
	BlobElements subject;
	REQUIRE(subject.size() == 0);

	nlohmann::json json(subject);
	REQUIRE(json.is_object());

	auto ret = json.get<BlobElements>();
	REQUIRE(ret.size() == 0);
}
