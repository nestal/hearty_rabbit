/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 5/27/18.
//

#include <catch.hpp>

#include "hrb/Timestamp.hh"

using namespace hrb;
using nlohmann::json;
using namespace std::chrono;

TEST_CASE("read and write round-trip timestamps from JSON", "[normal]")
{
	auto subject = Timestamp::now();
	json j;
	to_json(j, subject);

	Timestamp out;
	from_json(j, out);
	REQUIRE(out == subject);
}

TEST_CASE("automatically convert timestamps from/to JSON", "[normal]")
{
	auto subject = Timestamp::now();
	json json = subject;
	REQUIRE(json.get<Timestamp>() == subject);
}