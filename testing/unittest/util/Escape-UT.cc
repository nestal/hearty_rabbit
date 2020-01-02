/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/22/18.
//

#include <catch2/catch.hpp>

#include "common/util/Escape.hh"
#include "common/util/StringFields.hh"

#include <optional>

using namespace hrb;

TEST_CASE( "split left and right", "[normal]" )
{
	std::string_view in{"name=value"};
	REQUIRE(split_left(in, "=&;") == std::make_tuple("name", '='));
	REQUIRE(in == "value");
	REQUIRE(split_left(in, "&;")  == std::make_tuple("value", '\0'));
	REQUIRE(in.empty());
	REQUIRE(split_left(in, "=&;") == std::make_tuple("", '\0'));
	REQUIRE(in.empty());

	std::string_view url{"user/name/file?option"};
	REQUIRE(split_right(url, "/?") == std::make_tuple("option", '?'));
	REQUIRE(url == "user/name/file");
	REQUIRE(split_left(url, "/") == std::make_tuple("user", '/'));
	REQUIRE(url == "name/file");
	REQUIRE(split_right(url, "/") == std::make_tuple("file", '/'));
	REQUIRE(url == "name");
	REQUIRE(split_right(url, "/") == std::make_tuple("name", '\0'));
	REQUIRE(url == "");

	std::string option_only{"?option"};
}

TEST_CASE("get_fields_from_form_string", "[normal]")
{
	SECTION("simple 2 fields")
	{
		auto [name, other] = urlform.find("name=value&other=same", "name", "other");
		REQUIRE(name == "value");
		REQUIRE(other == "same");
	}
	SECTION("single empty field")
	{
		auto [only] = urlform.find("I am the only name", "I am the only name");
		REQUIRE(only == "");
	}
	SECTION("single empty field not found")
	{
		auto [only] = urlform.find("I am the only name", "not found");
		REQUIRE(only == "");
	}
	SECTION("some name without value")
	{
		auto [field] = urlform.find("some+name+without+value=", "some+name+without+value");
		REQUIRE(field == "");
	}
	SECTION("simple 3 fields")
	{
		std::string_view in{"username=nestal&password=123&something=else"};

		auto [username, password, something] = urlform.find(in, "username", "password", "something");
		REQUIRE(username == "nestal");
		REQUIRE(password == "123");
		REQUIRE(something == "else");
	}
	SECTION("4 fields: 1st one has no value")
	{
		auto [user, name, password, something] = urlform.find(
			"user&name=nestal;password=123+++++%%&something=$$%%else",
			"user", "name", "password", "something"
		);
		REQUIRE(user == "");
		REQUIRE(password == "123+++++%%");
		REQUIRE(something == "$$%%else");
	}
	SECTION("4 fields: find two of them")
	{
		auto [ink, pen] = urlform.find(
			"happy&birthday=2017-12-31;ink=blue&pen=good",
			"ink", "pen"
		);
		REQUIRE(ink == "blue");
		REQUIRE(pen == "good");
	}
	SECTION("4 fields: tailing ;")
	{
		auto [ink, pen] = urlform.find(
			"happy&2017-12-31;;ink=_I_don't_know_if_it_is_blue___&pen=good;;;;;;;",
			"ink", "pen"
		);
		REQUIRE(ink == "_I_don't_know_if_it_is_blue___");
		REQUIRE(pen == "good");
	}
}

TEST_CASE("URL parsing with tokenize()")
{
	using namespace std::literals;

	std::string_view url = "/blob/1234567890123456789012345678901234567890/as_svg";
	auto [empty, blob, hash, rendition] = tokenize<4>(url, "/");
	REQUIRE(empty == "");
	REQUIRE(blob == "blob");
	REQUIRE(hash == "1234567890123456789012345678901234567890");
	REQUIRE(rendition == "as_svg");

	// out-of-bound
	auto [empty2, blob2, hash2, rendition2, what2] = tokenize<5>(url, "/");
	REQUIRE(empty2 == "");
	REQUIRE(blob2 == "blob");
	REQUIRE(hash2 == "1234567890123456789012345678901234567890");
	REQUIRE(rendition2 == "as_svg");
	REQUIRE(what2 == "");

	// URL too long
	auto view_url = "/view/someone/path/to/a/valid/folder"sv;
	auto [empty3, view, user] = tokenize<3>(view_url, "/");
	REQUIRE(empty3 == "");
	REQUIRE(view == "view");
	REQUIRE(user == "someone");

	auto path = view_url.substr(view.size() + user.size() + 2);
	REQUIRE(path == "/path/to/a/valid/folder");

	// URL too long
	auto root_view = "/view/another"sv;
	auto [empty4, view4, user4] = tokenize<3>(root_view, "/");
	REQUIRE(empty4 == "");
	REQUIRE(view4 == "view");
	REQUIRE(user4 == "another");

	auto path4 = root_view.substr(view4.size() + user4.size() + 2);
	REQUIRE(path4 == "");
}

TEST_CASE("decode URI", "[normal]")
{
	REQUIRE(url_decode("%2Fview%2Fsumsum%2F") == "/view/sumsum/");
	REQUIRE(url_decode("%2Fview%2Fsumsum%2") == "/view/sumsum");
	REQUIRE(url_decode("%2Fview%OOsumsum%2F") == "/view");

	REQUIRE(url_encode("\xF0\x11") == "%F0%11");
}

TEST_CASE("basic find field with optional", "[normal]")
{
	auto [user, name, sum] = urlform.basic_find<std::optional<std::string>>("user=sum&sum=user", "user", "name", "sum");
	static_assert(std::is_same<decltype(user), std::optional<std::string>>::value);
	REQUIRE(user.has_value());
	REQUIRE(*user == "sum");
	REQUIRE(!name.has_value());
	REQUIRE(sum.has_value());
	REQUIRE(*sum == "user");

	auto [shield, sword, armor] = urlform.find_optional("shield=hylian&armor", "shield", "sword", "armor");
	static_assert(std::is_same<decltype(shield), std::optional<std::string_view>>::value);
	REQUIRE(shield.has_value());
	REQUIRE(*shield == "hylian");
	REQUIRE(!sword.has_value());
	REQUIRE(armor.has_value());
	REQUIRE(*armor == "");
}
