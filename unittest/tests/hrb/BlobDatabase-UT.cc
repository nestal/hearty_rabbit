/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 2/17/18.
//

#include <catch.hpp>

#include "hrb/BlobDatabase.hh"

using namespace hrb;

TEST_CASE("Open temp file", "[normal]")
{
	BlobDatabase subject{"/tmp"};
	auto tmp = subject.tmp_file();

	boost::system::error_code ec;

	char test[] = "hello world!!";
	auto count = tmp.write(test, sizeof(test), ec);
	REQUIRE(count == sizeof(test));
	REQUIRE(ec == boost::system::error_code{});

	tmp.seek(0, ec);
	REQUIRE(ec == boost::system::error_code{});

	char buf[80];
	count = tmp.read(buf, sizeof(buf), ec);
	REQUIRE(count == sizeof(test));
	REQUIRE(ec == boost::system::error_code{});
	REQUIRE(std::memcmp(test, buf, count) == 0);
}
