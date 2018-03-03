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

#include "hrb/BlobObject.hh"
#include "hrb/UploadFile.hh"
#include "util/MMap.hh"
#include "util/Magic.hh"

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
	tmp.open(".", bec);
	REQUIRE(!bec);
	tmp.write(mmap.data(), mmap.size(), bec);
	REQUIRE(!bec);

	return std::make_tuple(std::move(tmp), std::move(mmap));
}

TEST_CASE("upload non-image BlobObject", "[normal]")
{
	auto [tmp, src] = upload(__FILE__);

	std::error_code ec;
	auto subject = BlobObject::upload(std::move(tmp), Magic{}, {2048, 2048}, ec);
	REQUIRE(!ec);
	REQUIRE(subject.meta().mime() == "text/x-c");
	REQUIRE(subject.ID() != ObjectID{});

	subject.save(".", ec);
	REQUIRE(!ec);
	REQUIRE(fs::exists("master"));

	auto out = MMap::open("master", ec);
	REQUIRE(!ec);
	REQUIRE(out.size() == src.size());
	REQUIRE(std::memcmp(out.data(), src.data(), out.size()) == 0);
}

TEST_CASE("upload small image BlobObject", "[normal]")
{
	auto [tmp, src] = upload(image_path()/"black.jpg");

	std::error_code ec;
	auto subject = BlobObject::upload(std::move(tmp), Magic{}, {2048, 2048}, ec);
	REQUIRE(!ec);
	REQUIRE(subject.meta().mime() == "image/jpeg");
	REQUIRE(subject.ID() != ObjectID{});

	subject.save(".", ec);
	REQUIRE(!ec);
	REQUIRE(fs::exists("master"));

	auto out = MMap::open("master", ec);
	REQUIRE(!ec);
	REQUIRE(out.size() == src.size());
	REQUIRE(std::memcmp(out.data(), src.data(), out.size()) == 0);
}

TEST_CASE("upload big upright image BlobObject", "[normal]")
{
	auto [tmp, src] = upload(image_path()/"up_f_upright.jpg");

	std::error_code ec;
	auto subject = BlobObject::upload(std::move(tmp), Magic{}, {128, 128}, ec);
	REQUIRE(!ec);
	REQUIRE(subject.meta().mime() == "image/jpeg");
	REQUIRE(subject.ID() != ObjectID{});

	subject.save(".", ec);
	REQUIRE(!ec);
	REQUIRE(fs::exists("master"));

	auto out = MMap::open("master", ec);
	REQUIRE(!ec);
	REQUIRE(out.size() == src.size());
	REQUIRE(std::memcmp(out.data(), src.data(), out.size()) == 0);

	auto out128 = MMap::open("128x128", ec);
	REQUIRE(!ec);
	REQUIRE(out128.size() < src.size());
	REQUIRE(std::memcmp(out128.data(), src.data(), out128.size()) != 0);
}
