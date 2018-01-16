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

#include "net/RedisDriver.hh"

#include <iostream>

using namespace hrb;

TEST_CASE("Load BlobObject from file", "[normal]")
{
	BlobObject blob{__FILE__};

	ObjectID zero{};

	REQUIRE(std::memcmp(blob.ID().data, zero.data, zero.size) != 0);

	boost::asio::io_context ioc;
	RedisDriver db{ioc, "localhost", 6379};

	BlobObject copy;

	blob.save(db, [&db, &copy](auto& src)
	{
		// read it back
		copy.load(db, src.ID(), [&db](auto& copy)
		{
			std::cout << "reply is: " << copy.blob() << std::endl;

			db.disconnect();
		});
	});

	ioc.run();
}
