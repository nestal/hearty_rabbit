/*
	Copyright © 2020 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 2/1/2020.
//

#include <catch2/catch.hpp>

#include "util/Timestamp.hh"
#include "image/EXIF2.hh"
#include "util/MMap.hh"

#include "TestImages.hh"

using namespace hrb;

TEST_CASE("read date time from black.jpg", "[normal]")
{
	std::error_code ec;
	auto black = MMap::open(test_images / "black.jpg", ec);

	REQUIRE(!ec);

	EXIF2 subject{black.buffer()};

	auto dt = subject.date_time();
	REQUIRE(dt.has_value());

	Timestamp ts{std::chrono::time_point_cast<Timestamp::duration>(*dt)};
	REQUIRE(ts.http_format() == "Thu, 02 Jan 2020 21:42:02 GMT");
}
