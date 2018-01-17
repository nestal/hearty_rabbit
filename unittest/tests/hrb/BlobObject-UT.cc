/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/16/18.
//

#include <catch.hpp>

#include "hrb/BlobObject.hh"

#include "net/Redis.hh"

#include <iostream>

using namespace hrb;
using namespace hrb::redis;

TEST_CASE("Load BlobObject from file", "[normal]")
{
	BlobObject blob{__FILE__};

	ObjectID zero{};

	REQUIRE(std::memcmp(blob.ID().data, zero.data, zero.size) != 0);

	boost::asio::io_context ioc;
	Database db{ioc, "localhost", 6379};

	bool tested = false;
	BlobObject copy;
	SECTION("reopen file")
	{
		std::error_code ec;
		copy.open(boost::filesystem::path{__FILE__}.parent_path()/"Server-UT.cc", ec);
		REQUIRE(!ec);
	}

	blob.save(db, [&db, &copy, &tested](auto& src, bool success)
	{
		REQUIRE(success);
		REQUIRE(!src.empty());

		// read it back
		copy.load(db, src.ID(), [&db, &src, &tested](auto& copy, bool success)
		{
			REQUIRE(success);
			REQUIRE(!copy.empty());

			REQUIRE(src.blob() == copy.blob());
			REQUIRE(copy.blob().substr(0,2) == "/*");

			tested = true;
			db.disconnect();
		});
	});

	ioc.run();
	REQUIRE(tested);
}

TEST_CASE("Load non-exist BlobObject from redis", "[error]")
{
	boost::asio::io_context ioc;
	Database db{ioc, "localhost", 6379};

	bool tested = false;

	BlobObject blob;
	REQUIRE(blob.empty());

	blob.load(db, ObjectID{}, [&db, &tested](auto& blob, bool success)
	{
		REQUIRE(!success);
		REQUIRE(blob.empty());
		tested = true;
		db.disconnect();
	});

	ioc.run();
	REQUIRE(tested);
}
