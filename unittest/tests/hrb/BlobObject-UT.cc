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

TEST_CASE("upload BlobObject", "[normal]")
{
	std::error_code ec;
	auto src = MMap::open(fs::path{__FILE__}, ec);
	REQUIRE(!ec);

	boost::system::error_code bec;
	UploadFile tmp;
	tmp.open(".", bec);
	REQUIRE(!bec);
	tmp.write(src.data(), src.size(), bec);
	REQUIRE(!bec);

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
