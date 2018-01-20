/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/20/18.
//

#include <catch.hpp>

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

	MMap subject;
	subject.open(file.native_handle(), sec);
	REQUIRE(!sec);
	REQUIRE(subject.string_view().substr(0,2) == "/*");
}
