/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 6/3/18.
//

#include <catch2/catch.hpp>

#include "ServerInstance.hh"
#include "TestImages.hh"

#include "client/HRBClient.hh"
#include "client/HRBClient.ipp"

using namespace hrb;
using namespace std::chrono_literals;

TEST_CASE("simple client login", "[normal]")
{
	boost::asio::io_context ioc;
	ssl::context ctx{ssl::context::sslv23_client};

	int tested = 0;
	HRBClient subject{ioc, ctx, "localhost", ServerInstance::listen_https_port()};

	SECTION("login correct")
	{
		subject.login("sumsum", "bearbear", [&tested](auto err)
		{
			++tested;
			REQUIRE_FALSE(err);
		});

		REQUIRE(ioc.run_for(10s) > 0);
		REQUIRE(tested == 1);
		ioc.restart();

		// upload the source code of this unit test case
		subject.upload("", __FILE__, [&tested, &subject](auto intent, auto err)
		{
			tested++;

			REQUIRE_FALSE(err);
			REQUIRE_FALSE(intent.str().empty());
			REQUIRE(intent.blob().has_value());

			subject.get_blob("sumsum", "", *intent.blob(), "", [&tested](auto content, auto err)
			{
				tested++;
				REQUIRE_FALSE(err);
				REQUIRE_FALSE(content.empty());
			});

			subject.get_blob_meta("sumsum", "", *intent.blob(), [&tested](auto meta, auto err)
			{
				tested++;
				INFO("blob meta is " << meta);

				REQUIRE_FALSE(err);
				REQUIRE(meta["mime"] == "text/x-c");
			});
		});

		REQUIRE(ioc.run_for(10s) > 0);
		REQUIRE(tested == 4);
		ioc.restart();

		subject.scan_collections([&tested](auto coll_list, auto err)
		{
			REQUIRE_FALSE(err);
			auto it = coll_list.find("sumsum", "");
			REQUIRE(it != coll_list.end());
			REQUIRE(it->name() == "");

			++tested;
		});

		REQUIRE(ioc.run_for(10s) > 0);
		REQUIRE(tested == 5);
		ioc.restart();

		// list default collection
		subject.list_collection("", [&tested](auto coll, auto err)
		{
			REQUIRE_FALSE(err);
			REQUIRE(coll.name() == "");
			REQUIRE(coll.owner() == "sumsum");
			++tested;
		});

		REQUIRE(ioc.run_for(10s) > 0);
		REQUIRE(tested == 6);
		ioc.restart();

		auto lena = random_lena_bytes();
		subject.upload("lena", "lena.jpg", lena.begin(), lena.end(), [&tested](auto intent, auto err)
		{
			++tested;
			REQUIRE_FALSE(err);
			REQUIRE_FALSE(intent.str().empty());
			REQUIRE(intent.blob().has_value());
		});

		REQUIRE(ioc.run_for(10s) > 0);
		REQUIRE(tested == 7);
		ioc.restart();

		const char empty{};
		subject.upload("empty", "empty.jpg", &empty, &empty, [&tested](auto intent, auto err)
		{
			++tested;
			REQUIRE(err);
			REQUIRE(intent.action() == URLIntent::Action::none);
		});

		REQUIRE(ioc.run_for(10s) > 0);
		REQUIRE(tested == 8);
		ioc.restart();
	}
	SECTION("login incorrect")
	{
		subject.login("yungyung", "bunny", [&tested](auto err)
		{
			++tested;
			REQUIRE(err == hrb::Error::login_incorrect);
		});

		REQUIRE(ioc.run_for(10s) > 0);
		REQUIRE(tested == 1);
		ioc.restart();

		subject.list_collection("", [&tested](auto coll, auto err)
		{
			REQUIRE_FALSE(err);
			REQUIRE(coll.name() == "");
			REQUIRE(coll.owner() == "");
			++tested;
		});

		REQUIRE(ioc.run_for(10s) > 0);
		REQUIRE(tested == 2);
		ioc.restart();
	}
}
