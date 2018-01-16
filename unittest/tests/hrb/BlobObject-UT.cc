/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/16/18.
//

#include "hrb/BlobObject.hh"

#include <catch.hpp>

using namespace hrb;

TEST_CASE("Load BlobObject from file", "[normal]")
{
	BlobObject blob{__FILE__};

	ObjectID zero{};

	REQUIRE(std::memcmp(blob.ID().data, zero.data, zero.size) != 0);
}
