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

	Ownership subject{"test"};

	subject.link(
		*redis, "/", blobid, true, [&tested, redis, blobid](std::error_code ec)
		{
			REQUIRE(!ec);
			tested++;

			Ownership{"test"}.is_owned(*redis, "/", blobid, [&tested](bool owned, std::error_code ec)
			{
				REQUIRE(!ec);
				REQUIRE(owned);
				tested++;
			});
		}
	);
	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 2);
}
