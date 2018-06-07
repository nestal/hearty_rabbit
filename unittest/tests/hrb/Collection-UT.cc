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
#include "crypto/Random.hh"

#include <catch.hpp>

#include <iostream>

using namespace hrb;
using namespace std::chrono_literals;

TEST_CASE("simple CollEntry <-> JSON round-trip", "[normal]")
{
	CollEntry subject{Permission::public_(), "abc.txt", "text/plain", Timestamp{101s}};

	nlohmann::json json(subject);
	std::cout << json << std::endl;

	auto ret = json.get<CollEntry>();
	REQUIRE(subject.mime == ret.mime);
	REQUIRE(subject.filename == ret.filename);
	REQUIRE(subject.perm == ret.perm);
	REQUIRE(subject.timestamp == ret.timestamp);
}

TEST_CASE("simple Collection <-> JSON round-trip", "[normal]")
{
	auto cover = insecure_random<ObjectID>();

	Collection subject{"some_coll", "sumyung", nlohmann::json({"cover", cover})};
	subject.add_blob(insecure_random<ObjectID>(), {Permission::public_(), "abc.txt", "text/plain", Timestamp{101s}});
	subject.add_blob(insecure_random<ObjectID>(), {Permission::private_(), "image.jpeg", "image/jpeg", Timestamp{1h}});

	nlohmann::json json(subject);
	INFO("subject JSON: " << json);

	REQUIRE(subject.cover().has_value());
	REQUIRE(subject.cover() == cover);

	std::cout << json << std::endl;

	auto ret = json.get<Collection>();
	REQUIRE(subject.owner() == ret.owner());
	REQUIRE(subject.name() == ret.name());
	REQUIRE(subject.cover() == ret.cover());
}
