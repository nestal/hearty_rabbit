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

#include "image/RotateImage.hh"
#include "image/JPEG.hh"
#include "image/Exif2.hh"

#include "hrb/BlobMeta.hh"
#include "util/MMap.hh"
#include "util/FS.hh"
#include "util/Magic.hh"

#include <exiv2/exiv2.hpp>
#include <boost/beast/core/file.hpp>
#include <image/TurboBuffer.hh>

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

TEST_CASE("20x20 image can be auto-rotated but cropped", "[error]")
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

	auto cropped = MMap::open(out, ec);
	REQUIRE(!ec);

	int width=0, height=0, subsamp=0, colorspace=0;
	auto decomp = tjInitDecompress();
	REQUIRE(
		tjDecompressHeader3(decomp, static_cast<const unsigned char*>(cropped.data()), cropped.size(),
		&width, &height, &subsamp, &colorspace)
		== 0
	);
	tjDestroy(decomp);
	REQUIRE(width < 20);
	REQUIRE(height == 20);
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

TEST_CASE("resize image", "[normal]")
{
	std::error_code ec;
	auto img = MMap::open(fs::path{__FILE__}.parent_path()/"up_f_upright.jpg", ec);
	REQUIRE(!ec);

	JPEG subject{img.data(), img.size(), 100, 100};
	REQUIRE(subject.width() <= 100);
	REQUIRE(subject.height() <= 100);
	INFO("new width = " << subject.width());
	INFO("new height = " << subject.height());

	auto smaller = subject.compress(50);

	boost::beast::file out;
	boost::system::error_code bec;
	out.open("smaller.jpg", boost::beast::file_mode::write, bec);
	REQUIRE(!bec);
	out.write(smaller.data(), smaller.size(), bec);
	REQUIRE(!bec);

	JPEG sjpeg{smaller.data(), smaller.size(), 100, 100};
	REQUIRE(sjpeg.width() == subject.width());
	REQUIRE(sjpeg.height() == subject.height());
}

TEST_CASE("read exif", "[normal]")
{
	std::error_code ec;
	auto img = MMap::open(fs::path{__FILE__}.parent_path()/"up_f_upright.jpg", ec);
	REQUIRE(!ec);

	Exif2 subject{static_cast<unsigned char*>(img.data()), img.size()};
	auto orientation = subject.get(0x0112);
	REQUIRE(orientation);
	REQUIRE(orientation->value_offset == 1);
}
