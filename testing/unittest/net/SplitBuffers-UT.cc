/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 2/8/18.
//

#include <catch2/catch.hpp>

#include "net/SplitBuffers.hh"
#include "common/util/FS.hh"

using namespace hrb;
using Subject = hrb::SplitBuffers::value_type;

TEST_CASE("injecting script in HTML", "[normal]")
{
	std::error_code ec;
	auto html = hrb::MMap::open(fs::path{__FILE__}.parent_path() / "../../../lib/dynamic/index.html", ec);
	REQUIRE(ec == std::error_code{});

	std::string extra{R"({"field": "value"})"};
	std::string needle{"{/** dynamic json placeholder for dir **/}"};

	StringTemplate tmp{html.string()};
	tmp.replace(needle);

	Subject subject{tmp.begin(), tmp.end()};
	subject.set_extra(0, std::string{extra});
	REQUIRE(ec == std::error_code{});

	auto b = subject.data();
	REQUIRE(b.size() == 3);

	REQUIRE(std::string_view{static_cast<const char*>(b[0].data()), b[0].size()} == R"__(<!doctype html>
<html lang="en">
<head>
	<meta charset="utf-8">
	<script>var dir = )__");
	REQUIRE(std::string_view{static_cast<const char*>(b[1].data()), b[1].size()} == extra);
	REQUIRE(b[2].size() == html.size() - b[0].size() - needle.size());
}

TEST_CASE("non-HTML has no <head>, append at-the-end", "[normal]")
{
	std::error_code ec;
	auto css = hrb::MMap::open(fs::path{__FILE__}.parent_path() / "../../../lib/static/hearty_rabbit.css", ec);
	REQUIRE(ec == std::error_code{});

	std::string extra{"hahaha"};
	StringTemplate tmp{css.string()};
	tmp.replace("<head>", "");

	Subject subject{tmp.begin(), tmp.end()};
	subject.set_extra(0, std::string{extra});
	REQUIRE(ec == std::error_code{});

	auto b = subject.data();
	REQUIRE(b.size() == 2);

	REQUIRE(std::string_view{static_cast<const char*>(b[0].data()), b[0].size()} == css.string());
	REQUIRE(std::string_view{static_cast<const char*>(b[1].data()), b[1].size()} == extra);
}

TEST_CASE("Not change content", "[normal]")
{
	std::error_code ec;
	auto css = hrb::MMap::open(fs::path{__FILE__}.parent_path() / "../../../lib/static/hearty_rabbit.js", ec);
	REQUIRE(ec == std::error_code{});

	std::string_view vs[] = {css.string(), ""};
	Subject subject{std::begin(vs), std::end(vs)};

	REQUIRE(ec == std::error_code{});

	auto b = subject.data();
	REQUIRE(b.size() == 1);

	REQUIRE(std::string_view{static_cast<const char*>(b[0].data()), b[0].size()} == css.string());
}

TEST_CASE("Default constructor", "[normal]")
{
	Subject subject;
	REQUIRE(buffer_size(subject.data()) == 0);
}
