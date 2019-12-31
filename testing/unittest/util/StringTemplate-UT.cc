/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 4/22/18.
//

#include <catch2/catch.hpp>

#include "util/StringTemplate.hh"

using namespace hrb;

TEST_CASE("replace simple needle", "[normal]")
{
	StringTemplate tmp{"top {injection point #1} follow following {injection point #2} finale"};

	SECTION("inject #1 before #2")
	{
		tmp.replace("{injection point #1}");
		tmp.replace("{injection point #2}");
	}
	SECTION("inject #2 before #1")
	{
		tmp.replace("{injection point #2}");
		tmp.replace("{injection point #1}");
	}

	InstantiatedStringTemplate<2> subject{tmp.begin(), tmp.end()};
	subject.set_extra(0, "content1");
	subject.set_extra(1, "content2");
	REQUIRE(subject.str() == "top content1 follow following content2 finale");
}

TEST_CASE("replace subneedle", "[normal]")
{
	StringTemplate tmp{"top <script>/*placeholder*/</script> follow"};

	SECTION("replace subneedle with extra when found")
	{
		tmp.replace("<script>/*placeholder*/</script>", "/*placeholder*/");
		SECTION("not change extra")
		{
			InstantiatedStringTemplate<1> subject{tmp.begin(), tmp.end()};
			subject.set_extra(0, "var dir = {};");
			REQUIRE(subject.str() == "top <script>var dir = {};</script> follow");
		}
		SECTION("set another extra")
		{
			InstantiatedStringTemplate<1> subject{tmp.begin(), tmp.end()};
			subject.set_extra(0, "alert('oops');");
			REQUIRE(subject.str() == "top <script>alert('oops');</script> follow");
		}
	}
	SECTION("inject before")
	{
		tmp.inject_before("<script>/*placeholder*/</script>");
		InstantiatedStringTemplate<1> subject{tmp.begin(), tmp.end()};
		subject.set_extra(0, "<link></link>");
		REQUIRE(subject.str() == "top <link></link><script>/*placeholder*/</script> follow");
	}
	SECTION("inject after")
	{
		tmp.inject_after("<script>/*placeholder*/</script>");
		InstantiatedStringTemplate<1> subject{tmp.begin(), tmp.end()};
		subject.set_extra(0, "<body></body>");
		REQUIRE(subject.str() == "top <script>/*placeholder*/</script><body></body> follow");
	}
	SECTION("does not replace when subneedle not found")
	{
		tmp.replace("<script>/*placeholder*/</script>", "subneedle not found");
		REQUIRE(std::distance(tmp.begin(), tmp.end()) == 1);

		InstantiatedStringTemplate<0> subject{tmp.begin(), tmp.end()};
		REQUIRE(subject.str() == "top <script>/*placeholder*/</script> follow");
	}
	SECTION("instantiation is bigger than template")
	{
		tmp.replace("/*placeholder*/");
		REQUIRE(std::distance(tmp.begin(), tmp.end()) == 2);

		// we only have 2 segments in the template, but the instantiation expect 4
		InstantiatedStringTemplate<4> subject{tmp.begin(), tmp.end()};
		subject.set_extra(0, "<hi></hi>");
		REQUIRE(subject.str() == "top <script><hi></hi></script> follow");
	}
}
