/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 4/1/18.
//

#include <catch.hpp>
#include <iostream>

#include "hrb/BlobRequest.hh"

using namespace hrb;

TEST_CASE("BlobRequest move ctor")
{
	EmptyRequest get_blob;
	get_blob.method(http::verb::get);
	get_blob.target("/view/testuser/d83587387441dbd26616b532fe039fc0e9f4c927");

	BlobRequest subject{get_blob, "sumsum"};
	auto moved{std::move(subject)};

	REQUIRE(moved.blob().has_value());
	REQUIRE(*moved.blob() == *hex_to_object_id("d83587387441dbd26616b532fe039fc0e9f4c927"));
	REQUIRE(moved.owner() == "testuser");
	REQUIRE(moved.requester() == "sumsum");
}
