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
	std::error_code ec;
	auto html = hrb::MMap::open(boost::filesystem::path{__FILE__}.parent_path() / "../../../lib/dynamic/index.html", ec);
	REQUIRE(ec == std::error_code{});

	std::string extra{"<script></script>"};
	Subject subject{html.string(), extra};
	REQUIRE(ec == std::error_code{});

	auto buf = subject.data();
	static_assert(buf.size() == 3);

	auto [b1, b2, b3] = buf;
	REQUIRE(std::string_view{static_cast<const char*>(b1.data()), b1.size()} == "<!doctype html>\n<html lang=\"en\">\n<head>");
	REQUIRE(std::string_view{static_cast<const char*>(b2.data()), b2.size()} == extra);
	REQUIRE(b3.size() == html.size() - b1.size());
}

TEST_CASE("non-HTML has no <head>, append at-the-end", "[normal]")
{
	std::error_code ec;
	auto css = hrb::MMap::open(boost::filesystem::path{__FILE__}.parent_path() / "../../../lib/static/gallery.css", ec);
	REQUIRE(ec == std::error_code{});

	std::string extra{"hahaha"};
	Subject subject{css.string(), extra};
	REQUIRE(ec == std::error_code{});

	auto buf = subject.data();
	static_assert(buf.size() == 3);

	auto [b1, b2, b3] = buf;
	REQUIRE(std::string_view{static_cast<const char*>(b1.data()), b1.size()} == css.string());
	REQUIRE(std::string_view{static_cast<const char*>(b2.data()), b2.size()} == extra);
	REQUIRE(b3.size() == 0);
}
