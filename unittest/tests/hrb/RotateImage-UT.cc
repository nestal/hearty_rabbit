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
	const fs::path out = "rotated.jpeg";
	remove(out);

	std::error_code ec;
	auto rot90 = MMap::open(fs::path{__FILE__}.parent_path()/"up_f_rot90.jpg", ec);
	REQUIRE(!ec);
	REQUIRE(BlobMeta::deduce_meta(rot90.blob(), Magic{}).orientation() == 8);

	RotateImage subject;
	REQUIRE_NOTHROW(subject.auto_rotate(rot90.data(), rot90.size(), out, ec));
	REQUIRE(!ec);
	REQUIRE(exists(out));

	auto out_map = MMap::open(out, ec);
	REQUIRE(!ec);
	auto meta = BlobMeta::deduce_meta(out_map.blob(), Magic{});
	REQUIRE(meta.orientation() == 1);
}

TEST_CASE("20x20 image can be auto-rotated", "[error]")
{
	const fs::path out = "imperfect.jpeg";
	remove(out);

	std::error_code ec;
	auto rot90 = MMap::open(fs::path{__FILE__}.parent_path()/"black_20x20_orient6.jpg", ec);
	REQUIRE(!ec);
	REQUIRE(BlobMeta::deduce_meta(rot90.blob(), Magic{}).orientation() == 6);

	RotateImage subject;
	REQUIRE_NOTHROW(subject.auto_rotate(rot90.data(), rot90.size(), out, ec));
	REQUIRE(!ec);
	REQUIRE(exists(out));
}

TEST_CASE("png image cannot be auto-rotated", "[error]")
{
	const fs::path out = "not_exist.jpeg";
	remove(out);

	std::error_code ec;
	auto png = MMap::open(fs::path{__FILE__}.parent_path()/"black_20x20.png", ec);
	REQUIRE(!ec);

	RotateImage subject;
	REQUIRE_NOTHROW(subject.auto_rotate(png.data(), png.size(), out, ec));
	REQUIRE(!ec);
	REQUIRE(!exists(out));
}
/*
TEST_CASE("sony", "[error]")
{
	std::error_code ec;
	auto sony = MMap::open("/home/nestal/0941c8ba9b0906aaae9048a411c513470b67b395.jpg", ec);
	REQUIRE(!ec);
	REQUIRE(BlobMeta::deduce_meta(sony.blob(), Magic{}).orientation() == 6);
}
*/
