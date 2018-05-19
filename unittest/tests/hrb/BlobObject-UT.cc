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
	const Magic     m_magic;
	const fs::path  m_blob_path{"/tmp/BlobFile-UT"};
	const fs::path  m_image_path{fs::path{__FILE__}.parent_path().parent_path() / "image"};
};

TEST_CASE_METHOD(BlobFileUTFixture, "upload non-image BlobFile", "[normal]")
{
	RenditionSetting cfg;
	auto [tmp, src] = upload(__FILE__);

	std::error_code ec;
	BlobFile subject{std::move(tmp), m_blob_path, m_magic, ec};
	REQUIRE(!ec);
	REQUIRE(subject.ID() != ObjectID{});
	REQUIRE(subject.mime() == "text/x-c++");
	REQUIRE(fs::exists(m_blob_path/"master"));

	auto out = MMap::open(m_blob_path/"master", ec);
	REQUIRE(!ec);
	REQUIRE(out.size() == src.size());
	REQUIRE(std::memcmp(out.data(), src.data(), out.size()) == 0);

	SECTION("read back the original")
	{
		std::error_code read_ec;
		BlobFile subject2{m_blob_path, subject.ID()};

		REQUIRE(out.buffer() == subject2.rendition("master", cfg, read_ec).buffer());
		REQUIRE(!read_ec);
	}
	SECTION("read another rendition, but got the original")
	{
		std::error_code read_ec;
		BlobFile subject2{m_blob_path, subject.ID()};

		REQUIRE(out.buffer() == subject2.rendition("thumbnail", cfg, read_ec).buffer());
		REQUIRE(!read_ec);
	}
}

TEST_CASE_METHOD(BlobFileUTFixture, "upload small image BlobFile", "[normal]")
{
	auto [tmp, src] = upload(m_image_path/"black.jpg");

	std::error_code ec;
	BlobFile subject{std::move(tmp), m_blob_path, m_magic, ec};
	REQUIRE(!ec);
	REQUIRE(subject.ID() != ObjectID{});
	REQUIRE(subject.mime() == "image/jpeg");
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
	BlobFile subject{std::move(tmp), m_blob_path, m_magic, ec};
	REQUIRE(!ec);
	REQUIRE(subject.ID() != ObjectID{});
	REQUIRE(subject.mime() == "image/jpeg");
	REQUIRE(fs::exists(m_blob_path/"master"));

	auto out = MMap::open(m_blob_path/"master", ec);
	REQUIRE(!ec);
	REQUIRE(out.size() == src.size());
	REQUIRE(std::memcmp(out.data(), src.data(), out.size()) == 0);

	RenditionSetting cfg;
	cfg.add("128x128",   {128, 128});
	cfg.add("thumbnail", {64, 64});
	cfg.default_rendition("128x128");

	auto rend128 = subject.rendition(cfg.default_rendition(), cfg, ec);
	REQUIRE(!ec);
	REQUIRE(rend128.buffer() != src.buffer());

	auto out128 = MMap::open(m_blob_path/"128x128", ec);
	REQUIRE(!ec);
	REQUIRE(out128.size() < src.size());
	REQUIRE(std::memcmp(out128.data(), src.data(), out128.size()) != 0);

	BlobFile gen{m_blob_path, subject.ID()};
	auto gen_mmap = gen.rendition("thumbnail", cfg, ec);
	REQUIRE(gen_mmap.size() > 0);

	JPEG gen_jpeg{gen_mmap.buffer().data(), gen_mmap.size(), {64, 64}};
	REQUIRE(gen_jpeg.size().width() < 64);
	REQUIRE(gen_jpeg.size().height() < 64);
}

TEST_CASE_METHOD(BlobFileUTFixture, "upload big rot90 image as BlobFile", "[normal]")
{
	auto [tmp, src] = upload(m_image_path/"up_f_rot90.jpg");

	std::error_code ec;
	BlobFile subject{std::move(tmp), m_blob_path, m_magic, ec};

	RenditionSetting cfg;
	cfg.add("2048x2048",   {2048, 2048});
	cfg.default_rendition("2048x2048");

	auto rotated = subject.rendition(cfg.default_rendition(), cfg, ec);
	REQUIRE(!ec);
	REQUIRE(rotated.buffer() != src.buffer());

	JPEG gen_jpeg{rotated.buffer().data(), rotated.size(), {2048, 2048}};
	JPEG src_jpeg{src.buffer().data(),     src.size(),     {2048, 2048}};

	// make sure we don't accidentally down-smaple the image
	REQUIRE(gen_jpeg.size().width() < 2048);
	REQUIRE(gen_jpeg.size().height() < 2048);

	REQUIRE(gen_jpeg.size().width()  == src_jpeg.size().height());
	REQUIRE(gen_jpeg.size().height() == src_jpeg.size().width());
}

TEST_CASE("hex_to_object_id() error cases", "[error]")
{
	REQUIRE(!hex_to_object_id(""));
	REQUIRE(!hex_to_object_id("1"));
	REQUIRE(!hex_to_object_id("012345678901234567890123456789_123456789"));
	REQUIRE(!hex_to_object_id("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"));
	REQUIRE(!hex_to_object_id("0123456789012345678901234567890123456789AAAAAA"));
}
