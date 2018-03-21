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

#include "hrb/URLIntent.hh"

using namespace hrb;

TEST_CASE("path URL")
{
	URLIntent empty{""};
	REQUIRE(empty.action() == "");
	REQUIRE(empty.collection() == "");
	REQUIRE(empty.filename() == "");
	REQUIRE(empty.str() == "/");

	URLIntent slash{"/"};
	REQUIRE(slash.action() == "");
	REQUIRE(slash.user() == "");
	REQUIRE(slash.collection() == "");
	REQUIRE(slash.filename() == "");
	REQUIRE(slash.str() == "/");

	URLIntent no_path{"/view/file"};
	REQUIRE(no_path.action() == "view");
	REQUIRE(no_path.user() == "file");
	REQUIRE(no_path.collection() == "");
	REQUIRE(no_path.filename() == "");
	REQUIRE(no_path.str() == "/view/file/");

	URLIntent slash_view_slash{"/view/"};
	REQUIRE(slash_view_slash.action() == "view");
	REQUIRE(slash_view_slash.user() == "");
	REQUIRE(slash_view_slash.collection() == "");
	REQUIRE(slash_view_slash.filename() == "");
	REQUIRE(slash_view_slash.str() == "/view/");

	URLIntent slash_view{"/view"};
	REQUIRE(slash_view.action() == "view");
	REQUIRE(slash_view.user() == "");
	REQUIRE(slash_view.collection() == "");
	REQUIRE(slash_view.filename() == "");
	REQUIRE(slash_view.str() == "/view/");

	URLIntent slash_view_slash_user{"/view/sumyung"};
	REQUIRE(slash_view_slash_user.action() == "view");
	REQUIRE(slash_view_slash_user.user() == "sumyung");
	REQUIRE(slash_view_slash_user.collection() == "");
	REQUIRE(slash_view_slash_user.filename() == "");
	REQUIRE(slash_view_slash_user.str() == "/view/sumyung/");

	URLIntent slash_upload_slash_user_slash{"/upload/not_exists/"};
	REQUIRE(slash_upload_slash_user_slash.action() == "upload");
	REQUIRE(slash_upload_slash_user_slash.user() == "not_exists");
	REQUIRE(slash_upload_slash_user_slash.collection() == "");
	REQUIRE(slash_upload_slash_user_slash.filename() == "");
	REQUIRE(slash_upload_slash_user_slash.str() == "/upload/not_exists/");

	URLIntent slash_upload_slash_path_slash{"/upload/some/path/to/"};
	REQUIRE(slash_upload_slash_path_slash.action() == "upload");
	REQUIRE(slash_upload_slash_path_slash.user() == "some");
	REQUIRE(slash_upload_slash_path_slash.collection() == "path/to");
	REQUIRE(slash_upload_slash_path_slash.filename() == "");
	REQUIRE(slash_upload_slash_path_slash.str() == "/upload/some/path/to/");

	URLIntent slash_upload_slash_path_slash_filename{"/upload/path/to/upload/myfile.jpeg"};
	REQUIRE(slash_upload_slash_path_slash_filename.action() == "upload");
	REQUIRE(slash_upload_slash_path_slash_filename.user() == "path");
	REQUIRE(slash_upload_slash_path_slash_filename.collection() == "to/upload");
	REQUIRE(slash_upload_slash_path_slash_filename.filename() == "myfile.jpeg");
	REQUIRE(slash_upload_slash_path_slash_filename.str() == "/upload/path/to/upload/myfile.jpeg");

	// slash at the start and end of path will be removed
	URLIntent slash_upload_slash_path_slash_slash_filename{"/upload/path//myfile.jpeg"};
	REQUIRE(slash_upload_slash_path_slash_slash_filename.action() == "upload");
	REQUIRE(slash_upload_slash_path_slash_slash_filename.user() == "path");
	REQUIRE(slash_upload_slash_path_slash_slash_filename.collection() == "");
	REQUIRE(slash_upload_slash_path_slash_slash_filename.filename() == "myfile.jpeg");
	REQUIRE(slash_upload_slash_path_slash_slash_filename.str() == "/upload/path/myfile.jpeg");

	// slash at the start and end of path will be removed
	URLIntent slash_upload_slash_path_3slash_filename{"/upload/path///password"};
	REQUIRE(slash_upload_slash_path_3slash_filename.action() == "upload");
	REQUIRE(slash_upload_slash_path_3slash_filename.user() == "path");
	REQUIRE(slash_upload_slash_path_3slash_filename.collection() == "");
	REQUIRE(slash_upload_slash_path_3slash_filename.filename() == "password");
	REQUIRE(slash_upload_slash_path_3slash_filename.str() == "/upload/path/password");

	// slash at the start and end of path will be removed
	URLIntent path_with_2slashes{"/upload/path/something/////wrong/image.jpeg"};
	REQUIRE(path_with_2slashes.action() == "upload");
	REQUIRE(path_with_2slashes.user() == "path");
	REQUIRE(path_with_2slashes.collection() == "something/////wrong");
	REQUIRE(path_with_2slashes.filename() == "image.jpeg");
	REQUIRE(path_with_2slashes.str() == "/upload/path/something/////wrong/image.jpeg");

	URLIntent path_with_option{"/blob/user/path/id?rendition"};
	REQUIRE(path_with_option.action() == "blob");
	REQUIRE(path_with_option.user() == "user");
	REQUIRE(path_with_option.collection() == "path");
	REQUIRE(path_with_option.filename() == "id");
	REQUIRE(path_with_option.option() == "rendition");

	URLIntent path_with_question{"/blob/user/path/big_id?"};
	REQUIRE(path_with_question.action() == "blob");
	REQUIRE(path_with_question.user() == "user");
	REQUIRE(path_with_question.collection() == "path");
	REQUIRE(path_with_question.filename() == "big_id");
	REQUIRE(path_with_question.option() == "");

	URLIntent trim_action{"/action", "user", "", ""};
	REQUIRE(trim_action.str() == "/action/user/");

	URLIntent trim_action_fn{"/action", "user", "", "read.me"};
	REQUIRE(trim_action_fn.str() == "/action/user/read.me");

	URLIntent trim_user{"action", "/user", "", ""};
	REQUIRE(trim_user.str() == "/action/user/");
}
