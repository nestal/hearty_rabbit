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
	const char a1[] = "1234";

	ObjectID id;
	Query p{"query", a1, id};
	p.get([&a1, &id](auto&& query, std::size_t size, const char* const* values, const int* sizes, const int* formats)
	{
		REQUIRE(query == "query");
		REQUIRE(size == 2);

		REQUIRE(sizes[0] == 4);
		REQUIRE(sizes[1] == id.size());

		REQUIRE(formats[0] == 0);
		REQUIRE(formats[1] == 1);

		REQUIRE(values[0] == std::string(a1));
	});
}

TEST_CASE("postgres connect", "[normal]")
{
	boost::asio::io_context ioc;

	Session ss{ioc, "host=localhost"};

	auto blob = user_random<ObjectID>();

	int run = 0;
	ss.query("insert into blob_table (id, mime) values ($1, $2)", [&run](Result r)
	{
		std::cout << "result status: " << r.status() << "\n" << std::endl;
		run++;
	}, blob, "image/jpeg");

	// assume the randomly generated IDs are unique
	ss.query("select * from blob_table where id=$1", [&run](Result r)
	{
		std::cout << r.tuples() << std::endl;
		REQUIRE(r.tuples() == 1);
		run++;
	}, blob);

	ioc.run();
	REQUIRE(run == 2);
}
