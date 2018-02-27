/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/22/18.
//

#include "util/Escape.hh"

#include <catch.hpp>

using namespace hrb;

TEST_CASE( "split-front", "[normal]" )
{
	std::string_view in{"name=value"};
	REQUIRE(split_front(in, "=&;") == std::make_tuple("name", '='));
	REQUIRE(in == "value");
	REQUIRE(split_front(in, "&;")  == std::make_tuple("value", '\0'));
	REQUIRE(in.empty());

	REQUIRE(split_front(in, "=&;") == std::make_tuple("", '\0'));
	REQUIRE(in.empty());
}

TEST_CASE("get_fields_from_form_string", "[normal]")
{
	SECTION("simple 2 fields")
	{
		auto [name, other] = find_fields("name=value&other=same", "name", "other");
		REQUIRE(name == "value");
		REQUIRE(other == "same");
	}
	SECTION("single empty field")
	{
		auto [only] = find_fields("I am the only name", "I am the only name");
		REQUIRE(only == "");
	}
	SECTION("single empty field not found")
	{
		auto [only] = find_fields("I am the only name", "not found");
		REQUIRE(only == "");
	}
	SECTION("some name without value")
	{
		auto [field] = find_fields("some+name+without+value=", "some+name+without+value");
		REQUIRE(field == "");
	}
	SECTION("simple 3 fields")
	{
		std::string_view in{"username=nestal&password=123&something=else"};

		auto [username, password, something] = find_fields(in, "username", "password", "something");
		REQUIRE(username == "nestal");
		REQUIRE(password == "123");
		REQUIRE(something == "else");
	}
	SECTION("4 fields: 1st one has no value")
	{
		auto [user, name, password, something] = find_fields(
			"user&name=nestal;password=123+++++%%&something=$$%%else",
			"user", "name", "password", "something"
		);
		REQUIRE(user == "");
		REQUIRE(password == "123+++++%%");
		REQUIRE(something == "$$%%else");
	}
	SECTION("4 fields: find two of them")
	{
		auto [ink, pen] = find_fields(
			"happy&birthday=2017-12-31;ink=blue&pen=good",
			"ink", "pen"
		);
		REQUIRE(ink == "blue");
		REQUIRE(pen == "good");
	}
	SECTION("4 fields: tailing ;")
	{
		auto [ink, pen] = find_fields(
			"happy&2017-12-31;;ink=_I_don't_know_if_it_is_blue___&pen=good;;;;;;;",
			"ink", "pen"
		);
		REQUIRE(ink == "_I_don't_know_if_it_is_blue___");
		REQUIRE(pen == "good");
	}
}

TEST_CASE("URL parsing with tokenize()")
{
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
}
