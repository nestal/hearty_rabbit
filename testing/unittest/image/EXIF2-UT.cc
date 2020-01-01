/*
	Copyright © 2020 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/1/2020.
//

#include <catch2/catch.hpp>

#include "TestImages.hh"

#include "image/EXIF2.hh"
#include "util/MMap.hh"

using namespace hrb;

TEST_CASE("EXIF2 timestamp can be extraced", "[normal]")
{
	std::error_code ec;
	auto img = MMap::open(test_images/"DSC.jpg", ec);

	if (!ec)
	{
		EXIF2 exif2{img.buffer(), ec};
		REQUIRE(!ec);
	}
}
