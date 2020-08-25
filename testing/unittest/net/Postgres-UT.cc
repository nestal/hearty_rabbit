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

#include "net/Postgres.hh"
#include "util/Error.hh"

#include <iostream>

using namespace hrb::postgres;

TEST_CASE("postgres connect", "[normal]")
{
	boost::asio::io_context ioc;

	Session ss{ioc, ""};

	int run = 0;
	ss.query("insert into blob_table (id, mime) values ($1, $2)", [&run](Result r)
	{
		std::cout << r.fields() << std::endl;
		run++;
	}, "\\x12345678", "image/jpeg");
	ss.query("select * from blob_table", [&run](Result r)
	{
		std::cout << r.fields() << std::endl;
		run++;
	});
	ioc.run();
	REQUIRE(run == 2);
}
