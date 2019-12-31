/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 3/3/18.
//

#include <catch2/catch.hpp>

#include "hrb/UploadFile.hh"
#include "common/FS.hh"

using namespace hrb;

TEST_CASE("UploadFile tests")
{
	UploadFile subject;

	SECTION("open existing", "[normal]")
	{
		boost::system::error_code ec;
		subject.open(".", ec);
		REQUIRE(!ec);

//		subject.write()
	}
}
