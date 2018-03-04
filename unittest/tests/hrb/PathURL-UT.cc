/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 3/4/18.
//

#include <catch.hpp>

#include "hrb/PathURL.hh"

using namespace hrb;

TEST_CASE("path URL")
{
	PathURL empty{""};
	REQUIRE(empty.action() == "");
	REQUIRE(empty.path() == "/");
	REQUIRE(empty.filename() == "");

	PathURL slash{"/"};
	REQUIRE(slash.action() == "/");
	REQUIRE(slash.path() == "/");
	REQUIRE(slash.filename() == "");

	PathURL no_path{"/view/file"};
	REQUIRE(no_path.action() == "/view");
	REQUIRE(no_path.path() == "/");
	REQUIRE(no_path.filename() == "file");

	PathURL slash_view_slash{"/view/"};
	REQUIRE(slash_view_slash.action() == "/view");
	REQUIRE(slash_view_slash.path() == "/");
	REQUIRE(slash_view_slash.filename() == "");

	PathURL slash_view{"/view"};
	REQUIRE(slash_view.action() == "/view");
	REQUIRE(slash_view.path() == "/");
	REQUIRE(slash_view.filename() == "");

	PathURL slash_upload_slash_path_slash{"/upload/some/path/to/"};
	REQUIRE(slash_upload_slash_path_slash.action() == "/upload");
	REQUIRE(slash_upload_slash_path_slash.path() == "/some/path/to/");
	REQUIRE(slash_upload_slash_path_slash.filename() == "");

	PathURL slash_upload_slash_path_slash_filename{"/upload/path/to/upload/myfile.jpeg"};
	REQUIRE(slash_upload_slash_path_slash_filename.action() == "/upload");
	REQUIRE(slash_upload_slash_path_slash_filename.path() == "/path/to/upload/");
	REQUIRE(slash_upload_slash_path_slash_filename.filename() == "myfile.jpeg");
}
