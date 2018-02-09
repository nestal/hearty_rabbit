/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 2/9/18.
//

#include <catch.hpp>

#include "hrb/WebResources.hh"

#include "CheckResource.hh"

#include <boost/filesystem/path.hpp>
#include <iostream>

using namespace hrb;

TEST_CASE("static resource", "[normal]")
{
	auto web_root = boost::filesystem::path{__FILE__}.parent_path()/"../../../lib";
	WebResources lib{web_root};

	auto res = lib.find_dynamic("index.html", 11);
	REQUIRE(res.version() == 11);
	REQUIRE(res.result() == http::status::ok);
	REQUIRE(res[http::field::content_type] == "text/html");
	REQUIRE(res[http::field::cache_control] == "no-cache, no-store, must-revalidate");
	REQUIRE(check_resource_content(web_root / "dynamic/index.html", std::move(res)));
}
