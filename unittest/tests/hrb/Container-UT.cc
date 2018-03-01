/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 2/23/18.
//

#include <catch.hpp>
#include <iostream>

#include "hrb/Container.hh"
#include "hrb/BlobDatabase.hh"
#include "hrb/UploadFile.hh"

#include <rapidjson/document.h>

using namespace hrb;

TEST_CASE("Container tests", "[normal]")
{
	using namespace std::chrono_literals;

	boost::asio::io_context ioc;
	auto redis = redis::connect(ioc);

	std::error_code sec;
	BlobDatabase blobdb{"/tmp/BlobDatabase-UT"};
	UploadFile tmp;
	blobdb.prepare_upload(tmp, sec);

	boost::system::error_code ec;

	char test[] = "hello world!!";
	auto count = tmp.write(test, sizeof(test), ec);
	REQUIRE(count == sizeof(test));
	REQUIRE(ec == boost::system::error_code{});

	auto testid = blobdb.save(tmp, "hello.world", sec);

	int tested = 0;
	Container1::add(*redis, "test", testid, [&tested, redis, testid, &blobdb](std::error_code ec)
	{
		REQUIRE(!ec);

		Container1::is_member(*redis, "test", testid, [&tested](std::error_code ec, bool added)
		{
			REQUIRE(!ec);
			REQUIRE(added);
			tested++;
		});

		Container1::load(*redis, "test", [&tested, testid, &blobdb](std::error_code ec, Container1&& container)
		{
			REQUIRE(!ec);
			REQUIRE(container.name() == "test");
			REQUIRE(container.size() > 0);
			REQUIRE(!container.empty());

			REQUIRE(std::find(container.begin(), container.end(), testid) != container.end());

			auto json = container.serialize(blobdb);
			INFO("container json: " << json);
			REQUIRE(json.size() > 0);

			rapidjson::Document doc;
			doc.Parse(json.c_str(), json.size());
			REQUIRE(doc["name"].GetString() == std::string{"test"});

			tested++;
		});
	});
	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 2);
}
