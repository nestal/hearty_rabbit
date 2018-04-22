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

#include "net/SplitBuffers.hh"

#include <boost/filesystem.hpp>

using Subject = hrb::SplitBuffers::value_type;

TEST_CASE("injecting simple", "[normal]")
{
	Subject subject{"top {injection point #1} follow following {injection point #2} finale"};
	SECTION("inject #1 before #2")
	{
		subject.replace("{injection point #1}", "content1");
		subject.replace("{injection point #2}", "content2");
	}
	SECTION("inject #2 before #1")
	{
		subject.replace("{injection point #2}", "content2");
		subject.replace("{injection point #1}", "content1");
	}

	auto out_buf = subject.data();
	std::string out(boost::asio::buffer_size(out_buf), '_');
	boost::asio::buffer_copy(boost::asio::buffer(out), out_buf);

	REQUIRE(out == "top content1 follow following content2 finale");
}

TEST_CASE("injecting script in HTML", "[normal]")
{
	std::error_code ec;
	auto html = hrb::MMap::open(boost::filesystem::path{__FILE__}.parent_path() / "../../../lib/dynamic/index.html", ec);
	REQUIRE(ec == std::error_code{});

	std::string extra{R"({"field": "value"})"};
	std::string needle{"{/** dynamic json placeholder for dir **/}"};
	Subject subject{html.string(), needle, extra};
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
	auto css = hrb::MMap::open(boost::filesystem::path{__FILE__}.parent_path() / "../../../lib/static/hearty_rabbit.css", ec);
	REQUIRE(ec == std::error_code{});

	std::string extra{"hahaha"};
	Subject subject{css.string(), "<head>", extra};
	REQUIRE(ec == std::error_code{});

	auto b = subject.data();
	REQUIRE(b.size() == 3);

	REQUIRE(std::string_view{static_cast<const char*>(b[0].data()), b[0].size()} == css.string());
	REQUIRE(std::string_view{static_cast<const char*>(b[1].data()), b[1].size()} == extra);
	REQUIRE(b[2].size() == 0);
}

TEST_CASE("Not change content", "[normal]")
{
	std::error_code ec;
	auto css = hrb::MMap::open(boost::filesystem::path{__FILE__}.parent_path() / "../../../lib/static/hearty_rabbit.js", ec);
	REQUIRE(ec == std::error_code{});

	Subject subject{css.string()};
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
