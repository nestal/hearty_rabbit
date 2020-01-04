/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 6/7/18.
//

#include "common/hrb/BlobList.hh"
#include "common/hrb/Collection.hh"
#include "common/hrb/CollectionList.hh"
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

	Collection subject{"some_coll", "sumyung", nlohmann::json::object({{"cover", cover}})};
	subject.add_blob(insecure_random<ObjectID>(), {Permission::public_(), "abc.txt", "text/plain", Timestamp{101s}});
	subject.add_blob(insecure_random<ObjectID>(), {Permission::private_(), "image.jpeg", "image/jpeg", Timestamp{1h}});

	// don't use {} to construct nlohmann. it will produce an JSON array
	nlohmann::json json(subject);
	INFO("subject JSON: " << json);

	REQUIRE(subject.cover().has_value());
	REQUIRE(subject.cover() == cover);

	auto ret = json.get<Collection>();
	REQUIRE(subject.owner() == ret.owner());
	REQUIRE(subject.name() == ret.name());
	REQUIRE(subject.cover() == ret.cover());
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
	subject3.add("sumsum", "default", *subject.entries()[0].cover());
	subject3.add("sumsum", "some coll", *subject.entries()[1].cover());

	REQUIRE(subject == subject3);
	REQUIRE_FALSE(subject != subject3);
}

TEST_CASE("simple CollectionList <-> JSON round-trip", "[normal]")
{
	CollectionList subject;
	subject.add("sumsum", "default", insecure_random<ObjectID>());
	subject.add("sumsum", "some coll", insecure_random<ObjectID>());
	REQUIRE(subject.entries().size() == 2);

	// additional property besides cover
	subject.entries()[1].meta().emplace("field", "value");

	// don't use {} to construct nlohmann. it will produce an JSON array
	nlohmann::json json(subject);
	INFO("subject JSON: " << json);

	auto ret = json.get<CollectionList>();
	REQUIRE(ret.entries().size() == 2);
	REQUIRE(subject == ret);
	REQUIRE(ret.entries()[1].meta()["field"] == "value");
}

TEST_CASE("simple BlobList <-> JSON round-trip", "[normal]")
{
	BlobList subject;
	subject.add("sumsum", "coll", insecure_random<ObjectID>(), BlobInode{Permission::shared(), "abc.txt", "text/css"});

	REQUIRE(subject.size() == 1);
	for (auto&& e : subject.entries())
	{
		REQUIRE(e.owner == "sumsum");
		REQUIRE(e.coll == "coll");
		REQUIRE(e.entry.filename == "abc.txt");
		REQUIRE(e.entry.mime == "text/css");
		REQUIRE(e.entry.perm == Permission::shared());
	}
	subject.add("yung", "cool", insecure_random<ObjectID>(), BlobInode{Permission::private_(), "IMG_0102.JPG", "image/jpeg"});
	REQUIRE(subject.size() == 2);

	INFO("subject = " << subject.json());

	nlohmann::json json(std::move(subject));
	REQUIRE(subject.size() == 0);

	auto ret = json.get<BlobList>();
	REQUIRE(ret.size() == 2);
	INFO("ret = " << ret.json());

	// sort it by owner name for easy checking
	auto entries = ret.entries();
	std::sort(entries.begin(), entries.end(), [](auto&& e1, auto&& e2){return e1.owner < e2.owner;});
	REQUIRE(entries.size() == 2);

	REQUIRE(entries[0].owner == "sumsum");
	REQUIRE(entries[0].coll == "coll");
	REQUIRE(entries[0].entry.filename == "abc.txt");
	REQUIRE(entries[0].entry.mime == "text/css");
	REQUIRE(entries[0].entry.perm == Permission::shared());
	REQUIRE(entries[1].owner == "yung");
	REQUIRE(entries[1].coll == "cool");
	REQUIRE(entries[1].entry.filename == "IMG_0102.JPG");
	REQUIRE(entries[1].entry.mime == "image/jpeg");
	REQUIRE(entries[1].entry.perm == Permission::private_());
}

TEST_CASE("empty BlobList serialization", "[normal]")
{
	BlobList subject;
	REQUIRE(subject.size() == 0);

	nlohmann::json json(std::move(subject));
	REQUIRE(json.is_object());

	auto ret = json.get<BlobList>();
	REQUIRE(ret.size() == 0);
}
