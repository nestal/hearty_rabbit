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

#include "ServerInstance.hh"

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
	HRBClient subject{ioc, ctx, "localhost", ServerInstance::listen_https_port()};

	SECTION("login correct")
	{
		subject.login("sumsum", "bearbear", [&tested](auto err)
		{
			tested = true;
			REQUIRE_FALSE(err);
		});

		REQUIRE(ioc.run_for(10s) > 0);
		REQUIRE(tested);
		ioc.restart();

		tested = false;
		subject.scan_collections([&tested](auto json, auto err)
		{
			REQUIRE_FALSE(err);
			tested = true;
		});
	}
	SECTION("login incorrect")
	{
		subject.login("yungyung", "bunny", [&tested](auto err)
		{
			tested = true;
			REQUIRE(err == hrb::Error::login_incorrect);
		});
	}

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested);
	ioc.restart();

	tested = false;
	subject.list_collection("", [&tested](auto coll, auto err)
	{
		REQUIRE_FALSE(err);
		REQUIRE(coll.name() == "");
		tested = true;

//		for (auto&& ref : refs)
//			std::cout << ref.second.filename << std::endl;
	});

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested);
	ioc.restart();
}
