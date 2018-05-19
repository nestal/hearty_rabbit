/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 3/3/18.
//

#include <catch.hpp>

#include "hrb/BlobFile.hh"
#include "hrb/CollEntry.hh"
#include "hrb/UploadFile.hh"
#include "util/MMap.hh"
#include "util/Magic.hh"
#include "util/Configuration.hh"
#include "image/JPEG.hh"

using namespace hrb;

auto image_path()
{
	return fs::path{__FILE__}.parent_path().parent_path() / "image";
}

auto upload(const fs::path& file)
{
	std::error_code ec;
	auto mmap = MMap::open(file, ec);
	REQUIRE(!ec);

	boost::system::error_code bec;
	UploadFile tmp;
	tmp.open("/tmp/BlobFile-UT", bec);
	REQUIRE(!bec);
	tmp.write(mmap.data(), mmap.size(), bec);
	REQUIRE(!bec);

	return std::make_tuple(std::move(tmp), std::move(mmap));
}

TEST_CASE("upload non-image BlobFile", "[normal]")
{
	fs::remove_all("/tmp/BlobFile-UT");
	fs::create_directories("/tmp/BlobFile-UT");

	RenditionSetting cfg;
	auto [tmp, src] = upload(__FILE__);

	std::error_code ec;
	auto subject = BlobFile::upload(std::move(tmp), "/tmp/BlobFile-UT", Magic{}, cfg, "unittest.cc", ec);
	REQUIRE(!ec);
	REQUIRE(subject.ID() != ObjectID{});

	auto meta = subject.entry();
	REQUIRE(meta.mime() == "text/x-c");
	REQUIRE(meta.filename() == "unittest.cc");

//	subject.save("/tmp/BlobFile-UT", ec);
	REQUIRE(!ec);
	REQUIRE(fs::exists("/tmp/BlobFile-UT/master"));

	auto out = MMap::open("/tmp/BlobFile-UT/master", ec);
	REQUIRE(!ec);
	REQUIRE(out.size() == src.size());
	REQUIRE(std::memcmp(out.data(), src.data(), out.size()) == 0);

	SECTION("read back the original")
	{
		std::error_code read_ec;
		BlobFile subject2{"/tmp/BlobFile-UT", subject.ID()};

		REQUIRE(out.buffer() == subject2.rendition("master", cfg, read_ec).buffer());
		REQUIRE(!read_ec);
	}
	SECTION("read another rendition, but got the original")
	{
		std::error_code read_ec;
		BlobFile subject2{"/tmp/BlobFile-UT", subject.ID()};

		REQUIRE(out.buffer() == subject2.rendition("thumbnail", cfg, read_ec).buffer());
		REQUIRE(!read_ec);
	}
}

TEST_CASE("upload small image BlobFile", "[normal]")
{
	fs::remove_all("/tmp/BlobFile-UT");
	fs::create_directories("/tmp/BlobFile-UT");

	auto [tmp, src] = upload(image_path()/"black.jpg");

	std::error_code ec;
	auto subject = BlobFile::upload(std::move(tmp), "/tmp/BlobFile-UT", Magic{}, RenditionSetting{}, "black.jpeg", ec);
	REQUIRE(!ec);
	REQUIRE(subject.ID() != ObjectID{});

	auto meta = subject.entry();
	REQUIRE(meta.mime() == "image/jpeg");
	REQUIRE(meta.filename() == "black.jpeg");

//	subject.save("/tmp/BlobFile-UT", ec);
	REQUIRE(!ec);
	REQUIRE(fs::exists("/tmp/BlobFile-UT/master"));

	auto out = MMap::open("/tmp/BlobFile-UT/master", ec);
	REQUIRE(!ec);
	REQUIRE(out.size() == src.size());
	REQUIRE(std::memcmp(out.data(), src.data(), out.size()) == 0);
}

TEST_CASE("upload big upright image BlobFile", "[normal]")
{
	fs::remove_all("/tmp/BlobFile-UT");
	fs::create_directories("/tmp/BlobFile-UT");

	auto [tmp, src] = upload(image_path()/"up_f_upright.jpg");

	RenditionSetting cfg;
	cfg.add("128x128",   {128, 128});
	cfg.add("thumbnail", {64, 64});
	cfg.default_rendition("128x128");

	std::error_code ec;
	auto subject = BlobFile::upload(std::move(tmp), "/tmp/BlobFile-UT", Magic{}, cfg, "upright.jpeg", ec);
	REQUIRE(!ec);
	REQUIRE(subject.ID() != ObjectID{});

	auto meta = subject.entry();
	REQUIRE(meta.mime() == "image/jpeg");
	REQUIRE(meta.filename() == "upright.jpeg");

//	subject.save("/tmp/BlobFile-UT", ec);
	REQUIRE(!ec);
	REQUIRE(fs::exists("/tmp/BlobFile-UT/master"));

	auto out = MMap::open("/tmp/BlobFile-UT/master", ec);
	REQUIRE(!ec);
	REQUIRE(out.size() == src.size());
	REQUIRE(std::memcmp(out.data(), src.data(), out.size()) == 0);

	auto rend128 = subject.rendition(cfg.default_rendition(), cfg, ec);
	REQUIRE(!ec);
	REQUIRE(rend128.buffer() != src.buffer());

	auto out128 = MMap::open("/tmp/BlobFile-UT/128x128", ec);
	REQUIRE(!ec);
	REQUIRE(out128.size() < src.size());
	REQUIRE(std::memcmp(out128.data(), src.data(), out128.size()) != 0);

	BlobFile gen{"/tmp/BlobFile-UT", subject.ID()};
	auto gen_mmap = gen.rendition("thumbnail", cfg, ec);
	REQUIRE(gen_mmap.size() > 0);

	JPEG gen_jpeg{gen_mmap.buffer().data(), gen_mmap.size(), {64, 64}};
	REQUIRE(gen_jpeg.size().width() < 64);
	REQUIRE(gen_jpeg.size().height() < 64);
}

TEST_CASE("hex_to_object_id() error cases", "[error]")
{
	REQUIRE(!hex_to_object_id(""));
	REQUIRE(!hex_to_object_id("1"));
	REQUIRE(!hex_to_object_id("012345678901234567890123456789_123456789"));
	REQUIRE(!hex_to_object_id("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"));
	REQUIRE(!hex_to_object_id("0123456789012345678901234567890123456789AAAAAA"));
}
