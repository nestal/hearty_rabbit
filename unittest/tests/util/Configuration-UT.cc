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
#include <fstream>

#include "util/Configuration.hh"

using namespace hrb;

namespace {
const boost::filesystem::path current_src = boost::filesystem::path{__FILE__}.parent_path();
}

TEST_CASE( "--help command line parsing", "[normal]" )
{
	const char *argv[] = {"hearty_rabbit", "--help"};
	Configuration cfg{sizeof(argv)/sizeof(argv[1]), argv, nullptr};
	REQUIRE(cfg.help());
}

TEST_CASE( "Configuration without command line argument", "[error]" )
{
	REQUIRE_THROWS_AS(Configuration(0, nullptr, nullptr), Configuration::FileError);
}

TEST_CASE( "Load normal.json", "[normal]" )
{
	auto normal_json = (current_src / "normal.json").string();

	std::ifstream nj{normal_json};
	REQUIRE(nj);

	const char *argv[] = {"hearty_rabbit", "--cfg", normal_json.c_str()};
	Configuration cfg{sizeof(argv)/sizeof(argv[1]), argv, nullptr};
	REQUIRE(cfg.server_name() == "example.com");
	REQUIRE(cfg.listen_https().address() == boost::asio::ip::make_address("0.0.0.0"));
	REQUIRE(cfg.listen_http().address() == boost::asio::ip::make_address("0.0.0.0"));
	REQUIRE(cfg.listen_https().port() == 4433);
	REQUIRE(cfg.listen_http().port() == 8080);
}
