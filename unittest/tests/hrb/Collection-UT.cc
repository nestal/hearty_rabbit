/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 6/7/18.
//

#include "common/Collection.hh"
#include "common/CollectionList.hh"
#include "crypto/Random.hh"

#include <catch.hpp>

using namespace hrb;
using namespace std::chrono_literals;

TEST_CASE("simple CollEntry <-> JSON round-trip", "[normal]")
{
	CollEntry subject{Permission::public_(), "abc.txt", "text/plain", Timestamp{101s}};

	nlohmann::json json(subject);
	INFO("Subject JSON: " << json);

	auto ret = json.get<CollEntry>();
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
	subject.add("default", insecure_random<ObjectID>());
	subject.add("some coll", insecure_random<ObjectID>());

	CollectionList subject2;
	subject2.add("default", insecure_random<ObjectID>());
	subject2.add("some coll", insecure_random<ObjectID>());

	REQUIRE_FALSE(subject == subject2);
	REQUIRE(subject != subject2);

	CollectionList subject3;
	subject3.add("default", subject.entries()[0].cover());
	subject3.add("some coll", subject.entries()[1].cover());

	REQUIRE(subject == subject3);
	REQUIRE_FALSE(subject != subject3);
}

TEST_CASE("simple CollectionList <-> JSON round-trip", "[normal]")
{
	CollectionList subject;
	subject.add("default", insecure_random<ObjectID>());
	subject.add("some coll", insecure_random<ObjectID>());
	REQUIRE(subject.entries().size() == 2);

	// don't use {} to construct nlohmann. it will produce an JSON array
	nlohmann::json json(subject);
	INFO("subject JSON: " << json);

	auto ret = json.get<CollectionList>();
	REQUIRE(ret.entries().size() == 2);
	REQUIRE(subject == ret);
}
