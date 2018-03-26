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
#include <iostream>

#include "hrb/URLIntent.hh"

using namespace hrb;

TEST_CASE("path URL")
{
	URLIntent empty{""};
	REQUIRE(empty.action() == URLIntent::Action::none);
	REQUIRE(empty.collection() == "");
	REQUIRE(empty.filename() == "");
	REQUIRE(empty.str() == "/");
	REQUIRE_FALSE(empty.valid());

	URLIntent slash{"/"};
	REQUIRE(slash.action() == URLIntent::Action::home);
	REQUIRE(slash.user() == "");
	REQUIRE(slash.collection() == "");
	REQUIRE(slash.filename() == "");
	REQUIRE(slash.str() == "/");
	REQUIRE(slash.valid());

	URLIntent no_path{"/view/file"};
	REQUIRE(no_path.action() == URLIntent::Action::view);
	REQUIRE(no_path.user() == "file");
	REQUIRE(no_path.collection() == "");
	REQUIRE(no_path.filename() == "");
	REQUIRE(no_path.str() == "/view/file/");
	REQUIRE(no_path.valid());

	URLIntent view_filename{"/view/file/fname"};
	REQUIRE(view_filename.action() == URLIntent::Action::view);
	REQUIRE(view_filename.user() == "file");
	REQUIRE(view_filename.collection() == "fname");
	REQUIRE(view_filename.filename() == "");
	REQUIRE(view_filename.str() == "/view/file/fname/");
	REQUIRE(view_filename.valid());

	URLIntent slash_view_slash{"/view/"};
	REQUIRE(slash_view_slash.action() == URLIntent::Action::view);
	REQUIRE(slash_view_slash.user() == "");
	REQUIRE(slash_view_slash.collection() == "");
	REQUIRE(slash_view_slash.filename() == "");
	REQUIRE(slash_view_slash.str() == "/view/");
	REQUIRE_FALSE(slash_view_slash.valid());

	URLIntent slash_view{"/view"};
	REQUIRE(slash_view.action() == URLIntent::Action::view);
	REQUIRE(slash_view.user() == "");
	REQUIRE(slash_view.collection() == "");
	REQUIRE(slash_view.filename() == "");
	REQUIRE(slash_view.str() == "/view/");
	REQUIRE_FALSE(slash_view.valid());

	URLIntent slash_view_slash_user{"/view/sumyung"};
	REQUIRE(slash_view_slash_user.action() == URLIntent::Action::view);
	REQUIRE(slash_view_slash_user.user() == "sumyung");
	REQUIRE(slash_view_slash_user.collection() == "");
	REQUIRE(slash_view_slash_user.filename() == "");
	REQUIRE(slash_view_slash_user.str() == "/view/sumyung/");
	REQUIRE(slash_view_slash_user.valid());

	URLIntent slash_upload_slash_user_slash{"/upload/not_exists/"};
	REQUIRE(slash_upload_slash_user_slash.action() == URLIntent::Action::upload);
	REQUIRE(slash_upload_slash_user_slash.user() == "not_exists");
	REQUIRE(slash_upload_slash_user_slash.collection() == "");
	REQUIRE(slash_upload_slash_user_slash.filename() == "");
	REQUIRE(slash_upload_slash_user_slash.str() == "/upload/not_exists/");
	REQUIRE_FALSE(slash_upload_slash_user_slash.valid());

	URLIntent slash_upload_slash_path_slash{"/upload/some/path/to/"};
	REQUIRE(slash_upload_slash_path_slash.action() == URLIntent::Action::upload);
	REQUIRE(slash_upload_slash_path_slash.user() == "some");
	REQUIRE(slash_upload_slash_path_slash.collection() == "path/to");
	REQUIRE(slash_upload_slash_path_slash.filename() == "");
	REQUIRE(slash_upload_slash_path_slash.str() == "/upload/some/path/to/");
	REQUIRE_FALSE(slash_upload_slash_path_slash.valid());

	URLIntent slash_upload_slash_path_slash_filename{"/upload/path/to/upload/myfile.jpeg"};
	REQUIRE(slash_upload_slash_path_slash_filename.action() == URLIntent::Action::upload);
	REQUIRE(slash_upload_slash_path_slash_filename.user() == "path");
	REQUIRE(slash_upload_slash_path_slash_filename.collection() == "to/upload");
	REQUIRE(slash_upload_slash_path_slash_filename.filename() == "myfile.jpeg");
	REQUIRE(slash_upload_slash_path_slash_filename.str() == "/upload/path/to/upload/myfile.jpeg");
	REQUIRE(slash_upload_slash_path_slash_filename.valid());

	// slash at the start and end of path will be removed
	URLIntent slash_upload_slash_path_slash_slash_filename{"/upload/path//myfile.jpeg"};
	REQUIRE(slash_upload_slash_path_slash_slash_filename.action() == URLIntent::Action::upload);
	REQUIRE(slash_upload_slash_path_slash_slash_filename.user() == "path");
	REQUIRE(slash_upload_slash_path_slash_slash_filename.collection() == "");
	REQUIRE(slash_upload_slash_path_slash_slash_filename.filename() == "myfile.jpeg");
	REQUIRE(slash_upload_slash_path_slash_slash_filename.str() == "/upload/path/myfile.jpeg");
	REQUIRE(slash_upload_slash_path_slash_slash_filename.valid());

	// slash at the start and end of path will be removed
	URLIntent slash_upload_slash_path_3slash_filename{"/upload/path///password"};
	REQUIRE(slash_upload_slash_path_3slash_filename.action() == URLIntent::Action::upload);
	REQUIRE(slash_upload_slash_path_3slash_filename.user() == "path");
	REQUIRE(slash_upload_slash_path_3slash_filename.collection() == "");
	REQUIRE(slash_upload_slash_path_3slash_filename.filename() == "password");
	REQUIRE(slash_upload_slash_path_3slash_filename.str() == "/upload/path/password");
	REQUIRE(slash_upload_slash_path_3slash_filename.valid());

	// slash at the start and end of path will be removed
	URLIntent path_with_2slashes{"/upload/path/something/////wrong/image.jpeg"};
	REQUIRE(path_with_2slashes.action() == URLIntent::Action::upload);
	REQUIRE(path_with_2slashes.user() == "path");
	REQUIRE(path_with_2slashes.collection() == "something/////wrong");
	REQUIRE(path_with_2slashes.filename() == "image.jpeg");
	REQUIRE(path_with_2slashes.str() == "/upload/path/something/////wrong/image.jpeg");
	REQUIRE(path_with_2slashes.valid());

	URLIntent path_with_option{"/blob/user/path/id?rendition"};
	REQUIRE(path_with_option.action() == URLIntent::Action::blob);
	REQUIRE(path_with_option.user() == "user");
	REQUIRE(path_with_option.collection() == "path");
	REQUIRE(path_with_option.filename() == "id");
	REQUIRE(path_with_option.option() == "rendition");
	REQUIRE(path_with_2slashes.valid());

	URLIntent path_with_question{"/blob/user/path/big_id?"};
	REQUIRE(path_with_question.action() == URLIntent::Action::blob);
	REQUIRE(path_with_question.user() == "user");
	REQUIRE(path_with_question.collection() == "path");
	REQUIRE(path_with_question.filename() == "big_id");
	REQUIRE(path_with_question.option() == "");
	REQUIRE(path_with_question.valid());

	URLIntent trim_action{URLIntent::Action::blob, "user", "", ""};
	REQUIRE(trim_action.str() == "/blob/user/");

	URLIntent trim_action_fn{URLIntent::Action::upload, "user", "", "read.me"};
	REQUIRE(trim_action_fn.str() == "/upload/user/read.me");

	URLIntent trim_user{URLIntent::Action::view, "/user", "", ""};
	REQUIRE(trim_user.str() == "/view/user/");

	URLIntent js{"/lib/hearty_rabbit.js"};
	REQUIRE(js.action() == URLIntent::Action::lib);
	REQUIRE(js.filename() == "hearty_rabbit.js");

	URLIntent login{"/login"};
	REQUIRE(login.action() == URLIntent::Action::login);
	REQUIRE(login.valid());

	URLIntent login_fn{"/login/haha"};
	REQUIRE(login_fn.action() == URLIntent::Action::login);
	REQUIRE_FALSE(login_fn.valid());

	URLIntent upload_default{"/upload/nestal/DSC_1460.JPG"};
	REQUIRE(upload_default.action() == URLIntent::Action::upload);
	REQUIRE(upload_default.valid());
}
