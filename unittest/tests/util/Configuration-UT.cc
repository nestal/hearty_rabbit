/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/8/18.
//

#include <catch.hpp>

#include "util/Configuration.hh"

using namespace hrb;

TEST_CASE( "--help command line parsing", "[normal]" )
{
	const char *argv[] = {"hearty_rabbit", "--help"};
	Configuration cfg{sizeof(argv)/sizeof(argv[1]), argv, nullptr};
	REQUIRE(cfg.help());
}

TEST_CASE( "Configuration without command line argument", "[normal]" )
{
	REQUIRE_THROWS_AS(Configuration(0, nullptr, nullptr), Configuration::FileError);
}
