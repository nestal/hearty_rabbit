/*
	Copyright © 2020 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 5/1/2020.
//

#include <catch2/catch.hpp>

#include "hrbsync/CollectionComparison.hh"
#include "util/Random.hh"

using namespace hrb;

TEST_CASE("test sync collection", "[normal]")
{
	// some files to play with
	auto a = insecure_random<ObjectID>();
	auto b = insecure_random<ObjectID>();
	auto c = insecure_random<ObjectID>();
	auto d = insecure_random<ObjectID>();

	Collection local;
	local.add_blob(a, {Permission::public_(), "a.jpg"});
	local.add_blob(b, {Permission::public_(), "b.jpg"});

	Collection remote;
	remote.add_blob(a, {Permission::public_(), "a.jpg"});
	remote.add_blob(b, {Permission::public_(), "b.jpg"});
	remote.add_blob(c, {Permission::public_(), "c.jpg"});

	CollectionComparison subject{local, remote};
	REQUIRE(subject.download().size() == 1);
	REQUIRE(subject.download().find(c) != subject.download().end());
	REQUIRE(subject.upload().size() == 0);
}
