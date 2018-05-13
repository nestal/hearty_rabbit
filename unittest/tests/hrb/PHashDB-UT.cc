/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 5/13/18.
//

#include <catch.hpp>

#include "hrb/PHashDb.hh"
#include "image/PHash.hh"

#include "net/Redis.hh"

using namespace hrb;

TEST_CASE("find duplicated image in database", "[normal]")
{
	boost::asio::io_context ioc;
	auto redis = redis::connect(ioc);


}
