/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 3/4/18.
//

#include <catch2/catch.hpp>

#include "hrb/URLIntent.hh"
#include "util/StringFields.hh"

using namespace hrb;

TEST_CASE("standard path URL")
{
	URLIntent user{"/~bunny"};
	REQUIRE(user.type() == URLIntent::Type::user);
	REQUIRE(user.user());
	REQUIRE(user.user()->username == "bunny");
}
