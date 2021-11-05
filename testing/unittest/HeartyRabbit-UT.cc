/*
	Copyright © 2021 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/11/2021.
//

#include "HeartyRabbit.hh"
#include "crypto/Password.hh"

#include <catch2/catch.hpp>
#include <iostream>

using namespace hrb;

TEST_CASE("HRB2 test cases", "[normal]")
{
	boost::asio::io_context ios;
	hrb::HeartyRabbitServer srv{std::filesystem::current_path(), redis::connect(ios)};

	REQUIRE(srv.add_user("user", hrb::Password{"abc"}) == std::error_code());

	srv.login("user", hrb::Password{"abc"}, [](auto ec)
	{
		INFO(ec);
		REQUIRE(!ec);
	});

	ios.run();
}
