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

#include "hrb/Ownership.hh"
#include "hrb/BlobDatabase.hh"
#include "hrb/UploadFile.hh"
#include "crypto/Random.hh"

#include <rapidjson/document.h>

using namespace hrb;
	using namespace std::chrono_literals;

TEST_CASE("add blob to Ownership", "[normal]")
{
	auto blobid = insecure_random<ObjectID>();

	boost::asio::io_context ioc;
	auto redis = redis::connect(ioc);

	int tested = 0;

	Ownership::add_blob(
		*redis, "test", blobid, "/", [&tested, redis, blobid](std::error_code ec)
		{
			REQUIRE(!ec);
			tested++;
		}
	);
	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 1);
}

TEST_CASE("Ownership tests", "[normal]")
{
	boost::asio::io_context ioc;
	auto redis = redis::connect(ioc);

	std::error_code sec;
	BlobDatabase blobdb{"/tmp/BlobDatabase-UT", {2048,2048}};
	UploadFile tmp;
	blobdb.prepare_upload(tmp, sec);

	boost::system::error_code ec;

	char test[] = "hello world!!";
	auto count = tmp.write(test, sizeof(test), ec);
	REQUIRE(count == sizeof(test));
	REQUIRE(ec == boost::system::error_code{});

	auto testid = blobdb.save(std::move(tmp), "hello.world", sec);

	int tested = 0;
	Ownership::add(
		*redis, "test", testid, [&tested, redis, testid](std::error_code ec)
		{
			REQUIRE(!ec);

			Ownership::is_owned(
				*redis, "test", testid, [&tested](std::error_code ec, bool added)
				{
					REQUIRE(!ec);
					REQUIRE(added);
					tested++;
				}
			);

			Ownership::remove(
				*redis, "test", testid, [&tested, testid, redis](std::error_code ec)
				{
					REQUIRE(!ec);
					Ownership::is_owned(
						*redis, "test", testid, [&tested](std::error_code ec, bool present)
						{
							REQUIRE(!ec);
							REQUIRE(!present);
							tested++;
						}
					);
				}
			);
		}
	);
	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 2);
}
