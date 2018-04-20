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
#include "image/EXIF2.hh"

#include "hrb/CollEntry.hh"
#include "util/MMap.hh"
#include "util/FS.hh"
#include "util/Magic.hh"

#include <exiv2/exiv2.hpp>
#include <boost/beast/core/file.hpp>
#include <image/TurboBuffer.hh>

using namespace hrb;

long exiv2_orientation(boost::asio::const_buffer blob)
{
	auto ev2 = Exiv2::ImageFactory::open(static_cast<const unsigned char*>(blob.data()), blob.size());
    ev2->readMetadata();
	auto& exif = ev2->exifData();
	auto orientation = exif.findKey(Exiv2::ExifKey{"Exif.Image.Orientation"});
	return orientation != exif.end() ? orientation->toLong() : 1L;
}

TEST_CASE("auto rotate will change orientation=8 images to orientation=1", "[normal]")
{
	std::error_code ec;
	auto rot90 = MMap::open(fs::path{__FILE__}.parent_path()/"up_f_rot90.jpg", ec);
	REQUIRE(!ec);
	REQUIRE(exiv2_orientation(rot90.blob()) == 8);

	RotateImage subject;
	auto rotated = (subject.auto_rotate({static_cast<const unsigned char*>(rot90.data()), rot90.size()}, ec));
	REQUIRE(!ec);

	REQUIRE(!ec);
	REQUIRE(exiv2_orientation({rotated.data(), rotated.size()}) == 1);

	// check date time
	EXIF2 exif2{rot90.buffer().data(), rot90.size(), ec};
	REQUIRE(!ec);

	auto dt_field = exif2.get(rot90.buffer(), EXIF2::Tag::date_time);
	REQUIRE(dt_field);
	auto dt = exif2.get_value(rot90.buffer(), *dt_field);
	REQUIRE(dt.size() == 20); // required by EXIF2 spec
	REQUIRE(
		std::string_view{reinterpret_cast<const char*>(dt.data()), dt.size()} ==
		std::string_view{"1989:06:04 05:00:00\0", 20}
	);

	using namespace std::chrono;
	REQUIRE(
		duration_cast<seconds>(EXIF2::parse_datetime(dt).time_since_epoch()).count() == 612939600
	);
}

TEST_CASE("parsing datetime in EXIF")
{
	REQUIRE(EXIF2::parse_datetime({reinterpret_cast<const uint8_t*>("abc"), 4}) == EXIF2::time_point{});
}

TEST_CASE("20x20 image can be auto-rotated but cropped", "[error]")
{
	std::error_code ec;
	auto rot90 = MMap::open(fs::path{__FILE__}.parent_path()/"black_20x20_orient6.jpg", ec);
	REQUIRE(!ec);
	REQUIRE(exiv2_orientation(rot90.blob()) == 6);

	RotateImage subject;
	auto rotated = subject.auto_rotate({static_cast<const unsigned char*>(rot90.data()), rot90.size()}, ec);
	REQUIRE(!ec);

	int width=0, height=0, subsamp=0, colorspace=0;
	auto decomp = tjInitDecompress();
	REQUIRE(
		tjDecompressHeader3(decomp, rotated.data(), rotated.size(),
		&width, &height, &subsamp, &colorspace)
		== 0
	);
	tjDestroy(decomp);
	REQUIRE(width < 20);
	REQUIRE(height == 20);
}

TEST_CASE("png image cannot be auto-rotated", "[error]")
{
	std::error_code ec;
	auto png = MMap::open(fs::path{__FILE__}.parent_path()/"black_20x20.png", ec);
	REQUIRE(!ec);

	RotateImage subject;
	auto empty = subject.auto_rotate({static_cast<const unsigned char*>(png.data()), png.size()}, ec);
	REQUIRE(ec);
	REQUIRE(empty.empty());
}

TEST_CASE("resize image", "[normal]")
{
	std::error_code ec;
	auto img = MMap::open(fs::path{__FILE__}.parent_path()/"up_f_upright.jpg", ec);
	REQUIRE(!ec);

	JPEG subject{img.data(), img.size(), {100, 100}};
	REQUIRE(subject.size().width() <= 100);
	REQUIRE(subject.size().height() <= 100);
	INFO("new width = " << subject.size().width());
	INFO("new height = " << subject.size().height());

	auto smaller = subject.compress(50);

	boost::beast::file out;
	boost::system::error_code bec;
	out.open("smaller.jpg", boost::beast::file_mode::write, bec);
	REQUIRE(!bec);
	out.write(smaller.data(), smaller.size(), bec);
	REQUIRE(!bec);

	JPEG sjpeg{smaller.data(), smaller.size(), {100, 100}};
	REQUIRE(sjpeg.size() == subject.size());
}

TEST_CASE("read exif", "[normal]")
{
	std::error_code ec;
	auto mmap = MMap::open(fs::path{__FILE__}.parent_path()/"up_f_upright.jpg", ec);
	REQUIRE(!ec);

	// copy the memory as the mmap is read-only
	std::vector<unsigned char> img(
		static_cast<const unsigned char*>(mmap.data()),
		static_cast<const unsigned char*>(mmap.data()) + mmap.size()
	);

	EXIF2 subject{&img[0], img.size(), ec};
	REQUIRE(!ec);

	auto orientation = subject.get({&img[0], img.size()}, EXIF2::Tag::orientation);
	REQUIRE(orientation);
	REQUIRE(orientation->tag == 0x0112);
	REQUIRE(orientation->value_offset == 1);

	auto buf = subject.get_value({&img[0], img.size()}, *orientation);
	REQUIRE(buf.size() == sizeof(std::uint32_t));
	REQUIRE(*reinterpret_cast<const std::uint32_t *>(buf.data()) == 1);

	// set orientation to 8
	orientation->value_offset = 8;
	REQUIRE(subject.set(&img[0], *orientation));

	REQUIRE(exiv2_orientation({&img[0], img.size()}) == 8);
}

TEST_CASE("read all images successfully", "[normal]")
{
	std::unordered_map<std::string, EXIF2::Error> expected{
		{"XMP_only_6.jpg", EXIF2::Error::not_supported},
		{"black.jpg", EXIF2::Error::not_found},
		{"mspaint.jpg", EXIF2::Error::not_found}
	};

	for (auto&& img : fs::directory_iterator{fs::path{__FILE__}.parent_path()})
	{
		if (img.path().extension() == ".jpg")
		{
			INFO("loading " << img);
			std::error_code ec;
			auto mmap = MMap::open(img, ec);
			REQUIRE(!ec);

			EXIF2 subject{mmap.buffer().data(), mmap.size(), ec};
			INFO("EXIF2 of " << img << " load result = " << ec << " " <<  ec.message());

			auto eit = expected.find(img.path().filename().string());
			if (eit != expected.end())
				REQUIRE(ec == eit->second);
			else
			{
				REQUIRE(!ec);
				RotateImage rot;
				auto rotated = rot.auto_rotate(mmap.buffer(), ec);
				REQUIRE(!ec);

				if (!rotated.empty())
				{
					std::ofstream rot_out{"rot-" + img.path().filename().string(), std::ios::out | std::ios::binary};
					rot_out.rdbuf()->sputn(reinterpret_cast<const char *>(rotated.data()), rotated.size());
				}

				BufferView buf_out{
					rotated.empty() ? mmap.buffer().data() : rotated.data(),
					rotated.empty() ? mmap.size()          : rotated.size()
				};

				JPEG jpeg{buf_out.data(), buf_out.size(), {2048, 2048}};
				auto out_jpeg = jpeg.compress(70);

				std::ofstream out{img.path().filename().string(), std::ios::out | std::ios::binary};
				out.rdbuf()->sputn(reinterpret_cast<const char*>(out_jpeg.data()), out_jpeg.size());
			}
		}
	}
}
