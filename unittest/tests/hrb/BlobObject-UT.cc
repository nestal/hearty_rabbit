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

#include <bitset>
#include <chrono>

using namespace hrb;
using namespace hrb::redis;

TEST_CASE("Load BlobObject from file", "[normal]")
{
	BlobObject blob{__FILE__};
	INFO("blob = " << blob.name() << " " << blob.mime());

	REQUIRE(blob.name() == "BlobObject-UT.cc");
	REQUIRE(blob.mime() == "text/x-c");

	ObjectID zero{};

	REQUIRE(blob.ID() != zero);

	boost::asio::io_context ioc;
	Connection db{ioc, "localhost", 6379};

	std::bitset<2> tested{};

	blob.save(db, [&db, &tested](auto& src, auto ec)
	{
		REQUIRE(!ec);
		REQUIRE(!src.empty());

		auto key = src.redis_key();
		db.command([](auto, auto){}, "HDEL %b mime", key.data(), key.size());

		// read it back
		BlobObject::load(db, src.ID(), [&db, &src, &tested](auto& loaded, auto ec)
		{
			REQUIRE(!ec);
			REQUIRE(!loaded.empty());

			REQUIRE(src.blob() == loaded.blob());
			REQUIRE(loaded.blob().substr(0,2) == "/*");
			REQUIRE(loaded.name() == "BlobObject-UT.cc");
			REQUIRE(loaded.mime() == "text/x-c");

			SECTION("move operator")
			{
				BlobObject copy;
				copy.open(boost::filesystem::path{__FILE__}.parent_path()/"Server-UT.cc", ec);
				REQUIRE(!ec);
				REQUIRE(!copy.empty());

				copy = std::move(loaded);

				REQUIRE(loaded.blob().empty());
				REQUIRE(!copy.empty());
				REQUIRE(src.blob() == copy.blob());
				REQUIRE(copy.name() == "BlobObject-UT.cc");
			}

			tested.set(0);
			if (tested.all())
				db.disconnect();
		});

		// read again without loading the blob out
		BlobObject::load(db, src.ID(), __FILE__, [&db, &src, &tested](auto& loaded, auto ec)
		{
			REQUIRE(!ec);
			REQUIRE(!loaded.empty());
			REQUIRE(src.blob() == loaded.blob());
			REQUIRE(loaded.blob().substr(0,2) == "/*");
			REQUIRE(loaded.name() == "BlobObject-UT.cc");
			REQUIRE(loaded.mime() == "text/x-c");

			tested.set(1);
			if (tested.all())
				db.disconnect();
		});
	});

	using namespace std::chrono_literals;
	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested.all());
}

TEST_CASE("Load non-exist BlobObject from redis", "[error]")
{
	boost::asio::io_context ioc;
	Connection db{ioc, "localhost", 6379};

	bool tested = false;

	BlobObject blob;
	REQUIRE(blob.empty());

	BlobObject::load(db, ObjectID{}, [&db, &tested](auto& blob, auto ec)
	{
		REQUIRE(ec);
		REQUIRE(blob.empty());
		tested = true;
		db.disconnect();
	});

	using namespace std::chrono_literals;
	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested);
}

TEST_CASE("Create BlobObject from string_view", "[normal]")
{
	BlobObject subject("hello world!", "hello");
	INFO("hello world hash is " << subject.ID());

	REQUIRE(subject.name() == "hello");
	REQUIRE(subject.blob() == "hello world!");

	// test move ctor
	BlobObject moved{std::move(subject)};
	REQUIRE(subject.empty());
	REQUIRE(moved.name() == "hello");
	REQUIRE(moved.blob() == "hello world!");
}
