/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/14/18.
//

#include <catch.hpp>

#include "util/Configuration.hh"
#include "hrb/Server.hh"

namespace {

// Put all test data (i.e. the configuration files in this test) in the same directory as
// the source code, and use __FILE__ macro to find the test data.
// Expect __FILE__ to give the absolute path so the unit test can be run in any directory.
const boost::filesystem::path current_src = boost::filesystem::path{__FILE__}.parent_path();
}

using namespace hrb;

TEST_CASE("GET static resource", "[normal]")
{
	auto local_json = (current_src / "../../../etc/localhost.json").string();

	const char *argv[] = {"hearty_rabbit", "--cfg", local_json.c_str()};
	Configuration cfg{sizeof(argv)/sizeof(argv[1]), argv, nullptr};

	Server subject{cfg};

	REQUIRE(cfg.web_root() == (current_src/"../../../lib").lexically_normal());
	Request req;
	req.target("/index.html");

	subject.handle_https({}, std::move(req), [](auto&& response)
	{
		REQUIRE(response.result() == http::status::ok);
	});
}