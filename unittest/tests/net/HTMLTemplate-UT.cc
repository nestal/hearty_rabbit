/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 2/8/18.
//

#include <catch.hpp>

#include "net/HTMLTemplate.hh"

#include <boost/filesystem.hpp>

using Subject = hrb::HTMLTemplate::value_type;

TEST_CASE("injecting script in HTML", "[normal]")
{
	std::string extra{"<script></script>"};
	auto html = boost::filesystem::path{__FILE__}.parent_path() / "../../../lib/dynamic/index.html";

	std::error_code ec;
	Subject subject{html, extra, ec};
	REQUIRE(ec == std::error_code{});

	auto buf = subject.data();
	static_assert(buf.size() == 3);

	auto [b1, b2, b3] = buf;
	REQUIRE(std::string_view{static_cast<const char*>(b1.data()), b1.size()} == "<!doctype html>\n<html lang=\"en\">\n<head>");
	REQUIRE(std::string_view{static_cast<const char*>(b2.data()), b2.size()} == extra);
	REQUIRE(b3.size() == file_size(html) - b1.size());
}

TEST_CASE("non-HTML has no <head>, append at-the-end", "[normal]")
{
	std::string extra{"hahaha"};
	auto css = boost::filesystem::path{__FILE__}.parent_path() / "../../../lib/static/gallery.css";

	std::error_code ec;
	Subject subject{css, extra, ec};
	REQUIRE(ec == std::error_code{});

	auto buf = subject.data();
	static_assert(buf.size() == 3);

	auto [b1, b2, b3] = buf;
	REQUIRE(std::string_view{static_cast<const char*>(b1.data()), b1.size()} == hrb::MMap::open(css, ec).string());
	REQUIRE(std::string_view{static_cast<const char*>(b2.data()), b2.size()} == extra);
	REQUIRE(b3.size() == 0);
}
