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
#include "hrb/BlobMeta.hh"
#include "util/MMap.hh"
#include "util/FS.hh"
#include "util/Magic.hh"

#include <exiv2/exiv2.hpp>
#include <boost/beast/core/file.hpp>

using namespace hrb;

TEST_CASE("get orientation from exiv2", "[normal]")
{
	std::error_code ec;
	auto rot90 = MMap::open(fs::path{__FILE__}.parent_path()/"up_f_rot90.jpg", ec);
	REQUIRE(!ec);

	auto meta = BlobMeta::deduce_meta(rot90.blob(), Magic{});
	REQUIRE(meta.orientation() == 8);

	RotateImage subject;
	auto [buf, size] = subject.rotate(meta.orientation(), rot90.data(), rot90.size());

	boost::system::error_code bec;
	boost::beast::file rendition;
	rendition.open("rotated.jpeg", boost::beast::file_mode::write, bec);
	if (!bec)
		rendition.write(buf.get(), size, bec);
}
