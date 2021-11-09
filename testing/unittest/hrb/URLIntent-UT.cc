/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 3/4/18.
//

#include <catch2/catch.hpp>

#include "hrb/URLIntent.hh"
#include "util/StringFields.hh"

using namespace hrb;

TEST_CASE("standard path URL")
{
	URLIntent user{"/~bunny/sub/dir/?rend=abc"};
	REQUIRE(user.type() == URLIntent::Type::user);
	REQUIRE(user.user());
	REQUIRE(user.user()->username   == "bunny");
	REQUIRE(user.user()->path       == "/sub/dir/");
	REQUIRE(user.user()->rendition  == "abc");
	REQUIRE(user.str() == "/~bunny/sub/dir/?rend=abc");

	URLIntent user2{"/~bunny/somedir/filename"};
	REQUIRE(user2.type() == URLIntent::Type::user);
	REQUIRE(user2.user());
	REQUIRE(user2.user()->username   == "bunny");
	REQUIRE(user2.user()->path       == "/somedir/filename");
	REQUIRE(user2.user()->rendition  == "");
	REQUIRE(user2.str() == "/~bunny/somedir/filename");

	URLIntent user3{"/~turtle/image.jpeg?rend=1024"};
	REQUIRE(user3.type() == URLIntent::Type::user);
	REQUIRE(user3.user());
	REQUIRE(user3.user()->username   == "turtle");
	REQUIRE(user3.user()->path       == "/image.jpeg");
	REQUIRE(user3.user()->rendition  == "1024");
	REQUIRE(user3.str() == "/~turtle/image.jpeg?rend=1024");

	URLIntent login{"/session?create"};
	REQUIRE(login.type() == URLIntent::Type::session);
	REQUIRE(login.session());
	REQUIRE(login.session()->action == URLIntent::Session::Action::create);
	REQUIRE(login.str() == "/session?create");

	URLIntent lib1{"/lib/index.html"};
	REQUIRE(lib1.type() == URLIntent::Type::lib);
	REQUIRE(lib1.lib());
	REQUIRE(lib1.lib()->filename == "index.html");
	REQUIRE(lib1.str() == "/lib/index.html");

}
