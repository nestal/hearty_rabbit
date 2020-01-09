/*
	Copyright © 2020 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 8/1/2020.
//

#include "TestImages.hh"

#include "image/Image.hh"
#include "util/MMap.hh"

#include <catch2/catch.hpp>

using namespace hrb;

TEST_CASE("JPEG image metadata", "[normal]")
{
	std::error_code ec;
	auto mmap = MMap::open(test_images/"black.jpg", ec);
	REQUIRE(!ec);

	ImageMeta meta{mmap.buffer()};
	REQUIRE(meta.mime() == "image/jpeg");
	REQUIRE(meta.original_timestamp().has_value());
	REQUIRE(meta.original_timestamp()->time_since_epoch().count() > 0);
}
