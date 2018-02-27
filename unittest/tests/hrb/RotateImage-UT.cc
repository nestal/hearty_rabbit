/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 2/27/18.
//

#include <catch.hpp>

#include "hrb/RotateImage.hh"
#include "util/MMap.hh"
#include "util/FS.hh"

#include <exiv2/exiv2.hpp>
#include <boost/beast/core/file.hpp>

using namespace hrb;

TEST_CASE("rotate by 90 degree", "[normal]")
{
	std::error_code ec;
	auto upright = MMap::open(fs::path{__FILE__}.parent_path()/"up_f_upright.jpg", ec);
	REQUIRE(!ec);

	RotateImage subject;
	auto [buf, size] = subject.rotate(8, upright.data(), upright.size());
	auto [original, size2] = subject.rotate(6, buf.get(), size);


	boost::system::error_code bec;
	boost::beast::file rendition;
	rendition.open("rotated.jpeg", boost::beast::file_mode::write, bec);
	if (!bec)
		rendition.write(original.get(), size2, bec);

//	REQUIRE(size2 == upright.size());
//	REQUIRE(std::memcmp(upright.data(), original.get(), size2) == 0);
}
