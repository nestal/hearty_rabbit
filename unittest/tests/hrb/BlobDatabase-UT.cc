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
#include "hrb/BlobFile.hh"
#include "hrb/UploadFile.hh"
#include "net/MMapResponseBody.hh"
#include "util/Configuration.hh"
#include "util/Magic.hh"

using namespace hrb;

TEST_CASE("Open temp file", "[normal]")
{
	Configuration cfg;
	cfg.blob_path("/tmp/BlobDatabase-UT");

	fs::remove_all(cfg.blob_path());

	std::error_code sec;
	BlobDatabase subject{cfg};
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

	auto dest = subject.dest(subject.save(std::move(tmp), "testfile", sec).ID());
	INFO("save() error_code = " << sec << " " << sec.message());
	REQUIRE(!sec);
	REQUIRE(exists(dest/"master"));
	REQUIRE(file_size(dest/"master") == sizeof(test));

	auto res = subject.response(tmpid, 11, "image/jpeg", "");
	REQUIRE(res.result() == http::status::ok);
	REQUIRE(res[http::field::etag] != boost::string_view{});
}

TEST_CASE("Upload JPEG file to BlobDatabase", "[normal]")
{
	std::error_code ec;
	auto black = MMap::open(fs::path{__FILE__}.parent_path().parent_path() / "image" / "black.jpg", ec);
	REQUIRE(!ec);

	Configuration cfg;
	cfg.blob_path("/tmp/BlobDatabase-UT");

	BlobDatabase subject{cfg};
	UploadFile tmp;
	subject.prepare_upload(tmp, ec);
	REQUIRE(!ec);

	boost::system::error_code bec;
	tmp.write(black.data(), black.size(), bec);
	REQUIRE(!bec);

	auto id = subject.save(std::move(tmp), "black.jpg", ec).ID();
	REQUIRE(!ec);
}
