/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/20/18.
//

#include <catch2/catch.hpp>

#include "util/MMap.hh"

#include <boost/beast/core/file_posix.hpp>

using namespace hrb;

TEST_CASE("mmap self", "[normal]")
{
	boost::beast::file_posix file;

	boost::system::error_code bec;
	file.open(__FILE__, boost::beast::file_mode::read, bec);
	REQUIRE(!bec);

	std::error_code sec;

	auto subject = MMap::open(file.native_handle(), sec);
	REQUIRE(!sec);
	REQUIRE(subject.size() > 2);
	REQUIRE(subject.string().substr(0,2) == "/*");

	subject.clear();
	REQUIRE(subject.string().empty());
}

TEST_CASE("mmap non-exist", "[error]")
{
	std::error_code ec;

	auto subject = MMap::open(0x10000, ec);
	INFO("error_code is " << ec);
	REQUIRE(ec == std::errc::bad_file_descriptor);
}

TEST_CASE("Allocate and then re-open", "[normal]")
{
	std::error_code ec;

	auto subject = MMap::allocate(102, ec);
	REQUIRE(!ec);
	REQUIRE(subject.size() == 102);

	// clear() and then open() again
	subject.clear();
	REQUIRE(!subject.is_opened());

	boost::beast::file_posix file;

	boost::system::error_code bec;
	file.open(__FILE__, boost::beast::file_mode::read, bec);
	REQUIRE(!bec);

	subject = MMap::open(file.native_handle(), ec);
	REQUIRE(!ec);
	REQUIRE(subject.string().substr(0,2) == "/*");
}
