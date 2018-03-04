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
	REQUIRE(slash.user() == "");
	REQUIRE(slash.path() == "/");
	REQUIRE(slash.filename() == "");

	PathURL no_path{"/view/file"};
	REQUIRE(no_path.action() == "/view");
	REQUIRE(no_path.user() == "file");
	REQUIRE(no_path.path() == "/");
	REQUIRE(no_path.filename() == "");

	PathURL slash_view_slash{"/view/"};
	REQUIRE(slash_view_slash.action() == "/view");
	REQUIRE(slash_view_slash.user() == "");
	REQUIRE(slash_view_slash.path() == "/");
	REQUIRE(slash_view_slash.filename() == "");

	PathURL slash_view{"/view"};
	REQUIRE(slash_view.action() == "/view");
	REQUIRE(slash_view.user() == "");
	REQUIRE(slash_view.path() == "/");
	REQUIRE(slash_view.filename() == "");

	PathURL slash_view_slash_user{"/view/sumyung"};
	REQUIRE(slash_view_slash_user.action() == "/view");
	REQUIRE(slash_view_slash_user.user() == "sumyung");
	REQUIRE(slash_view_slash_user.path() == "/");
	REQUIRE(slash_view_slash_user.filename() == "");

	PathURL slash_upload_slash_user_slash{"/upload/not_exists/"};
	REQUIRE(slash_upload_slash_user_slash.action() == "/upload");
	REQUIRE(slash_upload_slash_user_slash.user() == "not_exists");
	REQUIRE(slash_upload_slash_user_slash.path() == "/");
	REQUIRE(slash_upload_slash_user_slash.filename() == "");

	PathURL slash_upload_slash_path_slash{"/upload/some/path/to/"};
	REQUIRE(slash_upload_slash_path_slash.action() == "/upload");
	REQUIRE(slash_upload_slash_path_slash.user() == "some");
	REQUIRE(slash_upload_slash_path_slash.path() == "/path/to/");
	REQUIRE(slash_upload_slash_path_slash.filename() == "");

	PathURL slash_upload_slash_path_slash_filename{"/upload/path/to/upload/myfile.jpeg"};
	REQUIRE(slash_upload_slash_path_slash_filename.action() == "/upload");
	REQUIRE(slash_upload_slash_path_slash_filename.user() == "path");
	REQUIRE(slash_upload_slash_path_slash_filename.path() == "/to/upload/");
	REQUIRE(slash_upload_slash_path_slash_filename.filename() == "myfile.jpeg");
}
