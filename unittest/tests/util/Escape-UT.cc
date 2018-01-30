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

TEST_CASE( "simple www-form", "[normal]" )
{
	auto count = 0;
	visit_form_string("name=value&other=same", [&count](auto name, auto value)
	{
		if (count == 0)
		{
			REQUIRE(name == "name");
			REQUIRE(value == "value");
			count++;
		}
		else
		{
			REQUIRE(name == "other");
			REQUIRE(value == "same");
		}
		return true;
	});
}

TEST_CASE( "no value in www-form", "[normal]" )
{
	auto called = false;
	visit_form_string("I am the only name", [&called](auto name, auto value)
	{
		REQUIRE(name == "I am the only name");
		REQUIRE(value.empty());
		REQUIRE(!called);
		called = true;
		return true;
	});

	called = false;
	visit_form_string("some+name+without+value=", [&called](auto name, auto value)
	{
		REQUIRE(name == "some+name+without+value");
		REQUIRE(value.empty());
		REQUIRE(!called);
		called = true;
		return true;
	});
}

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
	SECTION("3 fields")
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
}
