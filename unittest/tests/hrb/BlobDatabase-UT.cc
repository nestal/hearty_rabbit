/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 2/17/18.
//

#include <catch.hpp>

#include "hrb/BlobDatabase.hh"
#include "hrb/UploadFile.hh"

using namespace hrb;

TEST_CASE("Open temp file", "[normal]")
{
	fs::remove_all("/tmp/BlobDatabase-UT");

	std::error_code sec;
	BlobDatabase subject{"/tmp/BlobDatabase-UT"};
	UploadFile tmp;
	subject.prepare_upload(tmp, sec);

	boost::system::error_code ec;

	char test[] = "hello world!!";
	auto count = tmp.write(test, sizeof(test), ec);
	REQUIRE(count == sizeof(test));
	REQUIRE(ec == boost::system::error_code{});

	// seek to begin in order to read back the data
	tmp.seek(0, ec);
	REQUIRE(ec == boost::system::error_code{});


	char buf[80];
	count = tmp.read(buf, sizeof(buf), ec);
	REQUIRE(count == sizeof(test));
	REQUIRE(ec == boost::system::error_code{});
	REQUIRE(std::memcmp(test, buf, count) == 0);

	auto tmpid = tmp.ID();
	REQUIRE(tmpid != ObjectID{});
	REQUIRE(tmpid == tmp.ID());

	auto dest = subject.dest(subject.save(std::move(tmp), sec));
	INFO("save() error_code = " << sec << " " << sec.message());
	REQUIRE(!sec);
	REQUIRE(exists(dest));
	REQUIRE(file_size(dest) == sizeof(test));
}