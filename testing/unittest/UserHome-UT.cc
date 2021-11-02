/*
	Copyright © 2021 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/11/2021.
//

#include "UserHome.hh"

#include <catch2/catch.hpp>

using namespace hrb;

TEST_CASE("HRB2 test cases", "[normal]")
{
	UserHome subject{std::filesystem::path{__FILE__}.parent_path()};

	auto list = subject.list_directory("");
	REQUIRE(list.size() > 0);
}
