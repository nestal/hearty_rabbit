/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 3/3/18.
//

#include <catch2/catch.hpp>

#include "hrb/BlobFile.hh"
#include "hrb/CollEntryDB.hh"
#include "hrb/UploadFile.hh"

#include "common/util/MMap.hh"
#include "common/util/Magic.hh"
#include "util/Configuration.hh"

#include "image/Image.hh"
#include "TestImages.hh"

#include <config.hh>
#include <iostream>

using namespace hrb;

class BlobFileUTFixture
{
public:
	BlobFileUTFixture()
	{
		fs::remove_all(m_blob_path);
		fs::create_directories(m_blob_path);
	}

protected:
	auto upload(const fs::path& file)
	{
		std::error_code ec;
		auto mmap = MMap::open(file, ec);
		REQUIRE(!ec);

		boost::system::error_code bec;
		UploadFile tmp;
		tmp.open(m_blob_path, bec);
		REQUIRE(!bec);
		tmp.write(mmap.data(), mmap.size(), bec);
		REQUIRE(!bec);

		return std::make_tuple(std::move(tmp), std::move(mmap));
	}


protected:
	const fs::path  m_blob_path{"/tmp/BlobFile-UT"};
	const fs::path  m_image_path{hrb::test_images};
};

TEST_CASE_METHOD(BlobFileUTFixture, "upload non-image BlobFile", "[normal]")
{
	RenditionSetting cfg;
	auto [tmp, src] = upload(__FILE__);

	std::error_code ec;
	BlobFile subject{std::move(tmp), m_blob_path, ec};
	REQUIRE(!ec);
	REQUIRE(subject.ID() != ObjectID{});
	REQUIRE(subject.mime() == "text/x-c++");
	REQUIRE_FALSE(subject.phash().has_value());
	REQUIRE_FALSE(subject.is_image());
	REQUIRE(fs::exists(m_blob_path/"master"));
	REQUIRE(fs::exists(m_blob_path/"meta.json"));

	auto out = MMap::open(m_blob_path/"master", ec);
	REQUIRE(!ec);
	REQUIRE(out.size() == src.size());
	REQUIRE(std::memcmp(out.data(), src.data(), out.size()) == 0);

	SECTION("read back the original")
	{
		std::error_code read_ec;
		BlobFile subject2{m_blob_path, subject.ID()};

		REQUIRE(out.buffer() == subject2.load_master(read_ec).buffer());
		REQUIRE(!read_ec);
		REQUIRE(subject2.mime() == "text/x-c++");
		REQUIRE_FALSE(subject2.phash().has_value());
		REQUIRE_FALSE(subject2.is_image());
		REQUIRE(subject2.original_datetime() != Timestamp{});
	}
	SECTION("read another rendition, but got the original")
	{
		std::error_code read_ec;
		BlobFile subject2{m_blob_path, subject.ID()};

		REQUIRE(out.buffer() == subject2.rendition("thumbnail", cfg, std::string{constants::haarcascades_path}, read_ec).buffer());
		REQUIRE(!read_ec);
		REQUIRE(subject2.mime() == "text/x-c++");
		REQUIRE_FALSE(subject2.phash().has_value());
		REQUIRE_FALSE(subject2.is_image());
		REQUIRE(subject2.original_datetime() != Timestamp{});
	}
}

TEST_CASE_METHOD(BlobFileUTFixture, "upload small image BlobFile", "[normal]")
{
	auto [tmp, src] = upload(m_image_path/"black.jpg");

	std::error_code ec;
	BlobFile subject{std::move(tmp), m_blob_path, ec};
	REQUIRE(!ec);
	REQUIRE(subject.ID() != ObjectID{});
	REQUIRE(subject.mime() == "image/jpeg");
	REQUIRE(subject.is_image());
	REQUIRE(subject.original_datetime() != Timestamp{});
	REQUIRE(fs::exists(m_blob_path/"master"));

	auto out = MMap::open(m_blob_path/"master", ec);
	REQUIRE(!ec);
	REQUIRE(out.size() == src.size());
	REQUIRE(std::memcmp(out.data(), src.data(), out.size()) == 0);
}

TEST_CASE_METHOD(BlobFileUTFixture, "upload big upright image as BlobFile", "[normal]")
{
	auto [tmp, src] = upload(m_image_path/"up_f_upright.jpg");

	std::error_code ec;
	BlobFile subject{std::move(tmp), m_blob_path, ec};
	REQUIRE(!ec);
	REQUIRE(subject.ID() != ObjectID{});
	REQUIRE(subject.mime() == "image/jpeg");
	REQUIRE(subject.original_datetime() != Timestamp{});
	REQUIRE(fs::exists(m_blob_path/"master"));
	REQUIRE(fs::exists(m_blob_path/"meta.json"));

	auto out = MMap::open(m_blob_path/"master", ec);
	REQUIRE(!ec);
	REQUIRE(out.size() == src.size());
	REQUIRE(std::memcmp(out.data(), src.data(), out.size()) == 0);

	RenditionSetting cfg;
	cfg.add("128x128",   {128, 128});
	cfg.add("thumbnail", {64, 64});
	cfg.default_rendition("128x128");

	auto rend128 = subject.rendition(cfg.default_rendition(), cfg, std::string{constants::haarcascades_path}, ec);
	REQUIRE(!ec);
	REQUIRE(rend128.buffer() != src.buffer());

	auto out128 = MMap::open(m_blob_path/"128x128", ec);
	REQUIRE(!ec);
	REQUIRE(out128.size() < src.size());
	REQUIRE(std::memcmp(out128.data(), src.data(), out128.size()) != 0);

	BlobFile gen{m_blob_path, subject.ID()};
	auto gen_mmap = gen.rendition("thumbnail", cfg, std::string{constants::haarcascades_path}, ec);
	REQUIRE(gen_mmap.size() > 0);

	auto gen_jpeg = load_image(gen_mmap.buffer());
	REQUIRE(gen_jpeg.data);
	REQUIRE(!gen_jpeg.empty());
	REQUIRE(gen_jpeg.cols <= 64);
	REQUIRE(gen_jpeg.rows <= 64);
}

TEST_CASE_METHOD(BlobFileUTFixture, "upload big rot90 image as BlobFile", "[normal]")
{
	auto [tmp, src] = upload(m_image_path/"up_f_rot90.jpg");

	std::error_code ec;
	BlobFile subject{std::move(tmp), m_blob_path, ec};
	REQUIRE(subject.is_image());

	RenditionSetting cfg;
	cfg.add("2048x2048",   {2048, 2048});
	cfg.default_rendition("2048x2048");

	auto rotated = subject.rendition(cfg.default_rendition(), cfg, std::string{constants::haarcascades_path}, ec);
	REQUIRE_FALSE(ec);
	REQUIRE(rotated.buffer() != src.buffer());

	auto gen_jpeg = load_image(rotated.buffer());
	auto src_jpeg = load_image(src.buffer());

	REQUIRE(gen_jpeg.cols == 160);
	REQUIRE(gen_jpeg.rows == 192);
}

TEST_CASE_METHOD(BlobFileUTFixture, "upload lena.png as BlobFile", "[normal]")
{
	auto [tmp, src] = upload(m_image_path/"lena.png");

	std::error_code ec;
	BlobFile subject{std::move(tmp), m_blob_path, ec};
	REQUIRE_FALSE(ec);
	REQUIRE(subject.mime() == "image/png");
	REQUIRE(subject.is_image());
	REQUIRE(subject.phash().has_value());
	REQUIRE(subject.original_datetime() != Timestamp{});

	// open the blob using another object and compare meta data
	BlobFile subject2{m_blob_path, subject.ID()};
	REQUIRE(subject2.meta_json().is_object());
	REQUIRE(subject2.mime() == subject.mime());
	REQUIRE(subject2.phash() == subject.phash());

	// generate smaller rendition
	RenditionSetting cfg;
	cfg.add("256x256",   {256, 256});
	cfg.default_rendition("256x256");
	auto rend = subject2.rendition("256x256", cfg, std::string{constants::haarcascades_path}, ec);
	auto rend_mat = load_image(rend.buffer());
	REQUIRE(phash(rend_mat).compare(*subject2.phash()) == 1.0);
	REQUIRE(rend_mat.rows == 256);
	REQUIRE(rend_mat.cols == 256);
}

TEST_CASE_METHOD(BlobFileUTFixture, "upload image from camera as BlobFile", "[normal]")
{
	if (fs::exists(test_images/"DSC_7926.JPG"))
	{
		auto [tmp, src] = upload(m_image_path/"DSC_7926.JPG");
		std::error_code ec;
		BlobFile subject{std::move(tmp), m_blob_path, ec};
		REQUIRE(subject.original_datetime() != Timestamp{});

		auto dt = subject.original_datetime();
		auto tt = std::chrono::system_clock::to_time_t(dt);

		struct std::tm tm{};
		REQUIRE(::gmtime_r(&tt, &tm));

		// Wed Jul 18 10:00:48 2018
		INFO(asctime(&tm));

		REQUIRE(tm.tm_year == 118);     // year 2018
		REQUIRE(tm.tm_mon == 6);        // July
		REQUIRE(tm.tm_mday == 18);
		REQUIRE(tm.tm_hour == 10);
		REQUIRE(tm.tm_min == 0);
		REQUIRE(tm.tm_sec == 48);

	}
}

TEST_CASE("ObjectID::from_hex() error cases", "[error]")
{
	REQUIRE(!ObjectID::from_hex(""));
	REQUIRE(!ObjectID::from_hex("1"));
	REQUIRE(!ObjectID::from_hex("012345678901234567890123456789_123456789"));
	REQUIRE(!ObjectID::from_hex("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"));
	REQUIRE(!ObjectID::from_hex("0123456789012345678901234567890123456789AAAAAA"));
}
