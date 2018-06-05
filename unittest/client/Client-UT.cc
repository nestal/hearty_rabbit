/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 6/3/18.
//

#include <catch.hpp>

#include "client/GenericHTTPRequest.hh"
#include "client/HRBClient.hh"
#include "client/HRBClient.ipp"
#include <iostream>

using namespace hrb;
using namespace std::chrono_literals;

TEST_CASE("simple client login", "[normal]")
{
	boost::asio::io_context ioc;
	ssl::context ctx{ssl::context::sslv23_client};

	bool tested = false;

	HRBClient subject{ioc, ctx, "localhost", "4433"};
	subject.login("sumsum", "bearbear", [&tested](auto ec)
	{
		tested = true;
	});
	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested);
	ioc.restart();

	subject.list("", [](auto json)
	{
		std::cout << json << std::endl;
	});

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested);
	ioc.restart();
}
