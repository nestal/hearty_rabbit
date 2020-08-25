/*
	Copyright © 2020 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 25/8/2020.
//

#include <catch2/catch.hpp>

#include "hrb/ObjectID.hh"
#include "net/Postgres.hh"
#include "util/Error.hh"
#include "crypto/Random.hh"

#include <iostream>

using namespace hrb;
using namespace hrb::postgres;

TEST_CASE("postgres querry params", "[normal]")
{
	ObjectID id;
	QueryParams p{"1", id};
	REQUIRE(p.size() == 2);

	REQUIRE(p.sizes()[0] == 1);
	REQUIRE(p.sizes()[1] == id.size());

	REQUIRE(p.formats()[0] == 0);
	REQUIRE(p.formats()[1] == 1);
}

TEST_CASE("postgres connect", "[normal]")
{
	boost::asio::io_context ioc;

	Session ss{ioc, ""};

	int run = 0;
	ss.query("insert into blob_table (id, mime) values ($1, $2)", [&run](Result r)
	{
		std::cout << r.fields() << std::endl;
		run++;
	}, insecure_random<ObjectID>(), "image/jpeg");
	ss.query("select * from blob_table", [&run](Result r)
	{
		std::cout << r.fields() << std::endl;
		run++;
	});
	ioc.run();
	REQUIRE(run == 2);
}
