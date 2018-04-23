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

using namespace hrb;

TEST_CASE("web resource", "[normal]")
{
	auto web_root = boost::filesystem::path{__FILE__}.parent_path()/"../../../lib";
	WebResources lib{web_root};

	SECTION("dynamic resource")
	{
		// inject the same stuff to make sure the content remain unchanged
		auto res = lib.inject(
			http::status::ok,
			"{/** dynamic json placeholder for dir **/}",
			R"(<meta property="og:title" content="Hearty Rabbit" />)",
			11
		);
		REQUIRE(res.version() == 11);
		REQUIRE(res.result() == http::status::ok);
		REQUIRE(res[http::field::content_type] == "text/html");
		REQUIRE(check_resource_content(web_root / "dynamic/index.html", std::move(res)));
	}
	SECTION("static resource")
	{
		auto res = lib.find_static("logo.svg", "", 11);
		REQUIRE(res.version() == 11);
		REQUIRE(res.result() == http::status::ok);
		REQUIRE(res[http::field::content_type] == "image/svg+xml");
		REQUIRE(check_resource_content(web_root / "static/logo.svg", std::move(res)));

		// check etag is quoted
		auto etag = res[http::field::etag];
		REQUIRE(!etag.empty());
		REQUIRE(etag.front() == '\"');
		REQUIRE(etag.back() == '\"');

		// request with the right etag will give 304
		auto reres = lib.find_static("logo.svg", etag, 11);
		REQUIRE(reres.result() == http::status::not_modified);
	}
}
