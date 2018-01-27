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
#include "util/Random.hh"
#include "hrb/Server.hh"

#include <iostream>
#include <fstream>
#include <boost/beast/core/flat_buffer.hpp>

using namespace hrb;

namespace {

// Put all test data (i.e. the configuration files in this test) in the same directory as
// the source code, and use __FILE__ macro to find the test data.
// Expect __FILE__ to give the absolute path so the unit test can be run in any directory.
const boost::filesystem::path current_src = boost::filesystem::path{__FILE__}.parent_path();

template <typename Body, typename Allocator>
auto flatten_content(http::response<Body, http::basic_fields<Allocator>>&& res)
{
	boost::system::error_code ec;
	boost::beast::flat_buffer fbuf;

	// Spend a lot of time to get this line to compile...
	typename http::request<Body, http::basic_fields<Allocator>>::body_type::writer writer{res};
	while (auto buf = writer.get(ec))
	{
		if (!ec)
		{
			auto size = buffer_size(buf->first);
			buffer_copy(fbuf.prepare(size), buf->first);
			fbuf.commit(size);
		}
	}
	return fbuf;
}

template <typename ConstBuffer>
bool check_file_content(const boost::filesystem::path& file, ConstBuffer content)
{
	// open index.html and compare
	std::ifstream index{file.string()};
	char buf[1024];
	while (auto count = index.rdbuf()->sgetn(buf, sizeof(buf)))
	{
		auto result = std::memcmp(buf, content.data(), static_cast<std::size_t>(count));
		if (result != 0)
			return false;

		content += count;
	}

	return content.size() == 0;
}

}

TEST_CASE("GET static resource", "[normal]")
{
	auto local_json = (current_src / "../../../etc/localhost.json").string();

	const char *argv[] = {"hearty_rabbit", "--cfg", local_json.c_str()};
	Configuration cfg{sizeof(argv)/sizeof(argv[1]), argv, nullptr};

	Server subject{cfg};

	REQUIRE(cfg.web_root() == (current_src/"../../../lib").lexically_normal());
	Request req;

	SECTION("requesting index.html")
	{
		req.target("/index.html");

		subject.handle_https({}, std::move(req), [&cfg](auto &&res)
		{
			REQUIRE(res.result() == http::status::ok);
			auto content = flatten_content(std::move(res));
			REQUIRE(check_file_content(cfg.web_root() / "login.html", content.data()));
		});
	}
	SECTION("requesting other resources")
	{
		req.target("/");

		subject.handle_https({}, std::move(req), [&cfg](auto &&res)
		{
			REQUIRE(res.result() == http::status::ok);
			auto content = flatten_content(std::move(res));
			REQUIRE(check_file_content(cfg.web_root() / "login.html", content.data()));
		});
	}
}

TEST_CASE("Test random number", "[normal]")
{
	int rand{0};
	REQUIRE_NOTHROW(random(&rand, sizeof(rand)));
}