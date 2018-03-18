/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

#include "net/InsecureSession.hh"

#include <catch.hpp>

using namespace hrb;

TEST_CASE("redirect http to https", "[normal]")
{
	REQUIRE(InsecureSession::redirect("localhost:8080", 4433) == "https://localhost:4433");
	REQUIRE(InsecureSession::redirect("localhost:8080", 443)  == "https://localhost");
	REQUIRE(InsecureSession::redirect(":8080", 443)  == "https://");
	REQUIRE(InsecureSession::redirect(":8080", 100)  == "https://:100");
}
