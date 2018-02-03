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

#include "hrb/Server.hh"
#include "hrb/BlobObject.hh"

#include "crypto/Random.hh"
#include "util/Configuration.hh"

#include <iostream>
#include <fstream>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/buffers_to_string.hpp>

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
	typename http::response<Body, http::basic_fields<Allocator>>::body_type::writer writer{res};
	while (auto buf = writer.get(ec))
	{
		if (!ec)
		{
			auto size = buffer_size(buf->first);
			buffer_copy(fbuf.prepare(size), buf->first);
			fbuf.commit(size);
		}
		if (ec || !buf->second)
			break;
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

class MovedResponseChecker
{
public:
	MovedResponseChecker(std::string_view redirect) : m_redirect{redirect} {}

	template <typename Response>
	void operator()(Response&& res) const
	{
		REQUIRE(res.result() == http::status::moved_permanently);
		REQUIRE(res.at(http::field::location) == m_redirect);
		m_tested = true;
	}

	bool tested() const {return m_tested;}

private:
	std::string m_redirect;
	mutable bool m_tested = false;
};

class FileResponseChecker
{
public:
	FileResponseChecker(http::status status, boost::filesystem::path file) :
		m_status{status}, m_file{file}
	{
	}

	template <typename Response>
	void operator()(Response&& res) const
	{
		REQUIRE(res.result() == http::status::ok);
		auto content = flatten_content(std::move(res));
		REQUIRE(check_file_content(m_file, content.data()));

		m_tested = true;
	}

	bool tested() const {return m_tested;}

private:
	http::status m_status;
	boost::filesystem::path m_file;

	mutable bool m_tested{false};
};

}

TEST_CASE("GET static resource", "[normal]")
{
	auto local_json = (current_src / "../../../etc/localhost.json").string();

	const char *argv[] = {"hearty_rabbit", "--cfg", local_json.c_str()};
	Configuration cfg{sizeof(argv)/sizeof(argv[1]), argv, nullptr};

	Server subject{cfg};

	REQUIRE(cfg.web_root() == (current_src/"../../../lib").lexically_normal());
	Request req;

	SECTION("requesting something not exist")
	{
		FileResponseChecker checker{http::status::ok, cfg.web_root()/"login.html"};

		req.target("/something_not_exist.html");
		subject.handle_https(std::move(req), std::ref(checker));
		REQUIRE(checker.tested());
	}

	SECTION("Only allow login with POST: redirect GET request to login.html")
	{
		FileResponseChecker checker{http::status::ok, cfg.web_root()/"login.html"};

		req.target("/login");
		subject.handle_https(std::move(req), std::ref(checker));
		REQUIRE(checker.tested());
	}

	SECTION("Login Incorrect")
	{
		MovedResponseChecker checker{"/login_incorrect.html"};

		req.target("/login");
		req.method(http::verb::post);
		req.insert(http::field::content_type, "application/x-www-form-urlencoded");
		req.body() = "username=user&password=123";

		subject.handle_https(std::move(req), [&checker, &subject](auto&& res) mutable
		{
			checker(std::move(res));
			subject.disconnect_db();
		});
		subject.get_io_context().run();

		REQUIRE(checker.tested());
	}
	SECTION("Login incorrect without content-type")
	{
		MovedResponseChecker checker{"/login.html"};

		req.target("/login");
		req.method(http::verb::post);
		req.erase(http::field::content_type);
		REQUIRE(req[http::field::content_type] == "");
		req.body() = "username=user&password=123";
		subject.handle_https(std::move(req), std::ref(checker));
		REQUIRE(checker.tested());
	}

	SECTION("requesting other resources")
	{
		FileResponseChecker checker{http::status::ok, cfg.web_root()/"login.html"};

		req.target("/");
		subject.handle_https(std::move(req), std::ref(checker));
		REQUIRE(checker.tested());
	}

	SECTION("requesting invalid blob")
	{
		req.target("/blob/abc");
		subject.handle_https(std::move(req), [](auto&& res){REQUIRE(res.result() == http::status::not_found);});
	}

	SECTION("requesting empty blob ID")
	{
		req.target("/blob");
		subject.handle_https(std::move(req), [](auto&& res){REQUIRE(res.result() == http::status::not_found);});
	}

	SECTION("requesting good blob ID")
	{
		FileResponseChecker checker{http::status::ok, __FILE__};

		BlobObject blob{__FILE__};
		auto db = redis::connect(subject.get_io_context(), cfg.redis());

		blob.save(*db, [&subject, db, &req, &checker](auto&& blob, std::error_code ec)
		{
			db->disconnect();

			req.target("/blob/" + to_hex(blob.ID()));
			subject.handle_https(std::move(req), [&checker, &subject](auto&& res) mutable
			{
				checker(std::move(res));
				subject.disconnect_db();
			});
		});
		subject.get_io_context().run();
	}
}
