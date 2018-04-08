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

#include <json.hpp>
#include <fstream>

using namespace hrb;

namespace {

// Put all test data (i.e. the configuration files in this test) in the same directory as
// the source code, and use __FILE__ macro to find the test data.
// Expect __FILE__ to give the absolute path so the unit test can be run in any directory.
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
	REQUIRE_THROWS_AS(Configuration(0, nullptr, ""), Configuration::FileError);
}

TEST_CASE( "Load normal.json", "[normal]" )
{
	auto normal_json = (current_src / "normal.json").string();

	const char *argv[] = {"hearty_rabbit", "--cfg", normal_json.c_str()};
	Configuration cfg{sizeof(argv)/sizeof(argv[1]), argv, nullptr};

	REQUIRE(cfg.private_key() == (current_src/"key.pem").string());
	REQUIRE(cfg.cert_chain()  == (current_src/"certificate.pem").string());
	REQUIRE(cfg.web_root()    == "/usr/lib/hearty_rabbit");
	REQUIRE(cfg.server_name() == "example.com");
	REQUIRE(cfg.listen_https().address() == boost::asio::ip::make_address("0.0.0.0"));
	REQUIRE(cfg.listen_http().address() == boost::asio::ip::make_address("0.0.0.0"));
	REQUIRE(cfg.listen_https().port() == 4433);
	REQUIRE(cfg.listen_http().port() == 8080);
	REQUIRE(cfg.redis().address() == boost::asio::ip::make_address("192.168.1.1"));
	REQUIRE(cfg.redis().port() == 9181);
	REQUIRE(cfg.upload_limit() == 10*1024*1024);
	REQUIRE(cfg.session_length() == std::chrono::hours{1});
	REQUIRE(cfg.user_id() == 65535);
	REQUIRE(cfg.group_id() == 65535);
}

TEST_CASE( "Missing server name", "[error]" )
{
	auto error_json = (current_src / "missing_server_name.json").string();

	const char *argv[] = {"hearty_rabbit", "--cfg", error_json.c_str()};
	REQUIRE_THROWS_AS(Configuration(sizeof(argv)/sizeof(argv[1]), argv, nullptr), nlohmann::json::out_of_range);
}

TEST_CASE( "Bad web root", "[error]" )
{
	auto error_json = (current_src / "bad_web_root.json").string();

	const char *argv[] = {"hearty_rabbit", "--cfg", error_json.c_str()};
	REQUIRE_THROWS_AS(Configuration(sizeof(argv)/sizeof(argv[1]), argv, nullptr), nlohmann::json::type_error);
}

TEST_CASE( "Web root is .", "[normal]" )
{
	auto dot_json = (current_src / "web_root_is_dot.json").string();

	const char *argv[] = {"hearty_rabbit", "--cfg", dot_json.c_str()};
	Configuration subject{sizeof(argv)/sizeof(argv[1]), argv, nullptr};
	REQUIRE(subject.web_root() == current_src);
	REQUIRE(subject.redis().address() == boost::asio::ip::make_address("127.0.0.1"));
	REQUIRE(subject.redis().port() == 6379);
}

TEST_CASE( "Absolute path for certs", "[normal]" )
{
	auto dot_json = (current_src / "absolute_cert.json").string();

	const char *argv[] = {"hearty_rabbit", "--cfg", dot_json.c_str()};
	Configuration subject{sizeof(argv)/sizeof(argv[1]), argv, nullptr};
	REQUIRE(subject.web_root() == current_src);
	REQUIRE(subject.cert_chain() == "/etc/certificate.pem");
	REQUIRE(subject.private_key() == "/etc/key.pem");
}

TEST_CASE( "Set upload limit to 1.5MB", "[normal]" )
{
	auto upload_1_5mb = (current_src / "upload_limit_1.5mb.json").string();

	const char *argv[] = {"hearty_rabbit", "--cfg", upload_1_5mb.c_str()};
	Configuration subject{sizeof(argv)/sizeof(argv[1]), argv, nullptr};
	REQUIRE(subject.upload_limit() == 1.5*1024*1024);
	REQUIRE(subject.user_id() == 127);
	REQUIRE(subject.group_id() == 955);
}

TEST_CASE( "Multiple renditions", "[normal]" )
{
	auto file = (current_src / "rendition.json").string();

	const char *argv[] = {"hearty_rabbit", "--cfg", file.c_str()};
	Configuration subject{sizeof(argv)/sizeof(argv[1]), argv, nullptr};
	REQUIRE(subject.renditions().dimension(subject.renditions().default_rendition()) == Size2D{1024, 1024});
}

TEST_CASE( "Bad UID/GID", "[error]" )
{
	auto bad_gid = (current_src / "bad_gid.json").string();

	const char *argv[] = {"hearty_rabbit", "--cfg", bad_gid.c_str()};
	REQUIRE_THROWS_AS(Configuration(sizeof(argv)/sizeof(argv[1]), argv, nullptr), Configuration::InvalidUserOrGroup);

	auto bad_uid = (current_src / "bad_uid.json").string();

	const char *argv2[] = {"hearty_rabbit", "--cfg", bad_uid.c_str()};
	REQUIRE_THROWS_AS(Configuration(sizeof(argv2)/sizeof(argv2[1]), argv2, nullptr), Configuration::InvalidUserOrGroup);
}
