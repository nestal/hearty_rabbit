/*
	Copyright © 2023 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 7/10/23.
//

#include <catch2/catch.hpp>

#include "hrb/RequestTarget.hh"

using namespace hrb;

TEST_CASE("normal URL parsing", "[normal]")
{
	RequestTarget empty;
	REQUIRE(empty.path() == "/");
	REQUIRE(empty.action() == "");

	RequestTarget login{"/?login"};
	REQUIRE(login.path() == "/");
	REQUIRE(login.action() == "login");

	RequestTarget login_path{"/some/kind/of/path?login"};
	REQUIRE(login_path.path() == "/some/kind/of/path");
	REQUIRE(login_path.action() == "login");
}