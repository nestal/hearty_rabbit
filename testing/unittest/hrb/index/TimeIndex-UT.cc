/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 5/13/18.
//

#include <catch2/catch.hpp>

#include "hrb/index/TimeIndex.hh"

#include "crypto/Random.hh"
#include "util/Escape.hh"
#include "net/Redis.hh"

#include <iostream>

using namespace hrb;
using namespace std::chrono_literals;

TEST_CASE("add blob to TimeIndex", "[normal]")
{
	boost::asio::io_context ioc;
	auto redis = redis::connect(ioc);

	auto blob = insecure_random<ObjectID>();
	auto tested = false;

	TimeIndex subject{*redis};
	subject.add("sumsum", blob, std::chrono::system_clock::now());
}
