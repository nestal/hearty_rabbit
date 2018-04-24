/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

#include "util/BinStruct.hh"

#include <catch.hpp>

using namespace hrb;

TEST_CASE("BinStruct pack 3 fields", "[normal]")
{
	int a=0, b=1, c=2;

	BinStruct subject;
	subject.pack(a, b, c);

	REQUIRE(subject.size() == sizeof(int)*3);

	auto out_a = subject.unpack(-1);
	REQUIRE(out_a == a);
}
