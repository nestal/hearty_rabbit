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
#include "crypto/Password.hh"
#include "util/Configuration.hh"

#include <iostream>
#include <fstream>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/buffers_to_string.hpp>

using namespace hrb;
using namespace std::chrono_literals;

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

class Checker
{
public:
	bool tested() const {return m_tested;}

protected:
	void set_tested() const {m_tested = true;}

private:
	mutable bool m_tested = false;
};

class MovedResponseChecker : public Checker
{
public:
	MovedResponseChecker(std::string_view redirect) : m_redirect{redirect} {}

	template <typename Response>
	void operator()(Response&& res) const
	{
		REQUIRE(res.result() == http::status::moved_permanently);
		REQUIRE(res.at(http::field::location) == m_redirect);
		REQUIRE(res.version() == 11);
		set_tested();
	}

private:
	std::string m_redirect;
};

class GenericStatusChecker : public Checker
{
public:
	GenericStatusChecker(boost::beast::http::status status) : m_status{status} {}

	template <typename Response>
	void operator()(Response&& res) const
	{
		REQUIRE(res.result() == m_status);
		REQUIRE(res.version() == 11);
		set_tested();
	}

private:
	boost::beast::http::status m_status;
};

class FileResponseChecker : public Checker
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
		REQUIRE(res.version() == 11);

		set_tested();
	}

private:
	http::status m_status;
	boost::filesystem::path m_file;
};

SessionID create_session(std::string_view username, std::string_view password, const Configuration& cfg)
{
	boost::asio::io_context ioc;
	auto db = redis::connect(ioc, cfg.redis());

	std::promise<SessionID> result;

	add_user(username, Password{password}, *db, [&result, db, username, password](std::error_code ec)
	{
		REQUIRE(!ec);

		verify_user(username, Password{password}, *db, [&result, db](std::error_code ec, const SessionID& id)
		{
			REQUIRE(!ec);
			result.set_value(id);
			db->disconnect();
		});
	});
	ioc.run();
	return result.get_future().get();
}

}

TEST_CASE("GET static resource", "[normal]")
{
	auto local_json = (current_src / "../../../etc/localhost.json").string();

	const char *argv[] = {"hearty_rabbit", "--cfg", local_json.c_str()};
	Configuration cfg{sizeof(argv)/sizeof(argv[1]), argv, nullptr};

	auto session = create_session("testuser", "password", cfg);

	Server subject{cfg};

	REQUIRE(cfg.web_root() == (current_src/"../../../lib").lexically_normal());
	Request req;
	req.version(11);

	SECTION("Request login.html success without login")
	{
		FileResponseChecker checker{http::status::ok, cfg.web_root()/"login.html"};

		req.target("/login.html");
		subject.handle_https(std::move(req), std::ref(checker));
		REQUIRE(checker.tested());
	}

	SECTION("Request logo.svg success without login")
	{
		FileResponseChecker checker{http::status::ok, cfg.web_root()/"logo.svg"};

		req.target("/logo.svg");
		subject.handle_https(std::move(req), std::ref(checker));
		REQUIRE(checker.tested());
	}

	SECTION("Request index.html failed without login")
	{
		MovedResponseChecker checker{"/login.html"};

		req.target("/index.html");
		subject.handle_https(std::move(req), std::ref(checker));
		REQUIRE(subject.get_io_context().run_for(10s) > 0);
		REQUIRE(checker.tested());
	}

	SECTION("Request index.html success with login")
	{
		FileResponseChecker checker{http::status::ok, cfg.web_root()/"index.html"};

		req.target("/index.html");
		req.insert(boost::beast::http::field::cookie, set_cookie(session));
		subject.handle_https(std::move(req), [&checker, &subject](auto&& res)
		{
			checker(std::move(res));
			subject.disconnect_db();
		});

		REQUIRE(subject.get_io_context().run_for(10s) > 0);
		REQUIRE(checker.tested());
	}

	SECTION("requesting something not exist")
	{
		MovedResponseChecker checker{"/login.html"};

		req.target("/something_not_exist.html");
		subject.handle_https(std::move(req), std::ref(checker));
		REQUIRE(subject.get_io_context().run_for(10s) > 0);
		REQUIRE(checker.tested());
	}

	SECTION("Only allow login with POST: redirect GET request to login.html")
	{
		MovedResponseChecker checker{"/login.html"};

		req.target("/login");
		subject.handle_https(std::move(req), std::ref(checker));
		REQUIRE(subject.get_io_context().run_for(10s) > 0);
		REQUIRE(checker.tested());
	}

	SECTION("Login Incorrect")
	{
		MovedResponseChecker login_incorrect{"/login_incorrect.html"};
		MovedResponseChecker invalid_login{"/login.html"};
		Checker *expect{nullptr};

		req.target("/login");
		req.method(http::verb::post);
		req.body() = "username=user&password=123";

		SECTION("correct content_type")
		{
			req.insert(http::field::content_type, "application/x-www-form-urlencoded");
			subject.handle_https(std::move(req), [&login_incorrect, &subject](auto&& res)
			{
				REQUIRE(res[http::field::cookie] == "");
				login_incorrect(std::move(res));
				subject.disconnect_db();
			});
			REQUIRE(subject.get_io_context().run_for(10s) > 0);
			expect = &login_incorrect;
		}
		SECTION("without content_type")
		{
			req.erase(http::field::content_type);
			REQUIRE(req[http::field::content_type] == "");
			subject.handle_https(std::move(req), std::ref(invalid_login));
			expect = &invalid_login;
		}
		SECTION("without incorrect content_type")
		{
			req.insert(http::field::content_type, "text/plain");
			subject.handle_https(std::move(req), std::ref(invalid_login));
			expect = &invalid_login;
		}

		REQUIRE(expect != nullptr);
		REQUIRE(expect->tested());
	}

	SECTION("requesting other resources")
	{
		MovedResponseChecker checker{"/login.html"};

		req.target("/");
		subject.handle_https(std::move(req), std::ref(checker));
		REQUIRE(subject.get_io_context().run_for(10s) > 0);
		REQUIRE(checker.tested());
	}

	SECTION("requesting invalid blob")
	{
		GenericStatusChecker valid_session{http::status::not_found};
		MovedResponseChecker invalid_session{"/login.html"};
		Checker *expected{nullptr};

		SECTION("blob ID too short")
		{
			req.target("/blob/abc");
			SECTION("with valid session")
			{
				req.insert(boost::beast::http::field::cookie, set_cookie(session));
				subject.handle_https(std::move(req), std::ref(valid_session));
				expected = &valid_session;
			}
			SECTION("with invalid session")
			{
				subject.handle_https(std::move(req), std::ref(invalid_session));
				expected = &invalid_session;
			}
		}
		SECTION("empty blob ID")
		{
			req.target("/blob");
			SECTION("with valid session")
			{
				req.insert(boost::beast::http::field::cookie, set_cookie(session));
				subject.handle_https(std::move(req), std::ref(valid_session));
				expected = &valid_session;
			}
			SECTION("with invalid session")
			{
				subject.handle_https(std::move(req), std::ref(invalid_session));
				expected = &invalid_session;
			}
		}

		REQUIRE(subject.get_io_context().run_for(10s) > 0);
		REQUIRE(expected->tested());
	}

	SECTION("requesting good blob ID")
	{
		FileResponseChecker checker{http::status::ok, __FILE__};

		BlobObject blob{__FILE__};
		auto db = redis::connect(subject.get_io_context(), cfg.redis());

		blob.save(*db, [&subject, db, &req, &checker, session](auto&& blob, std::error_code ec)
		{
			db->disconnect();

			req.target("/blob/" + to_hex(blob.ID()));
			req.insert(boost::beast::http::field::cookie, set_cookie(session));
			subject.handle_https(std::move(req), [&checker, &subject](auto&& res)
			{
				REQUIRE(res.at(http::field::content_type) == "text/x-c++");
				checker(std::move(res));
				subject.disconnect_db();
			});
		});
		REQUIRE(subject.get_io_context().run_for(10s) > 0);
		REQUIRE(checker.tested());
	}
}
