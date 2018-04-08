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

#include "CheckResource.hh"

#include "hrb/Server.hh"
#include "hrb/SessionHandler.ipp"
#include "hrb/Ownership.ipp"

#include "crypto/Random.hh"
#include "crypto/Password.hh"
#include "util/Configuration.hh"

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/buffers_to_string.hpp>

using namespace hrb;
using namespace std::chrono_literals;

namespace {

// Put all test data (i.e. the configuration files in this test) in the same directory as
// the source code, and use __FILE__ macro to find the test data.
// Expect __FILE__ to give the absolute path so the unit test can be run in any directory.
const boost::filesystem::path current_src = boost::filesystem::path{__FILE__}.parent_path();

class Checker
{
public:
	bool tested() const {return m_tested;}

	template <typename T>
	auto operator[](T&& field) const { return m_header[std::forward<T>(field)]; }

protected:
	void set_tested(const http::header<false>& header) const {m_tested = true; m_header = header;}

private:
	mutable bool m_tested = false;
	mutable http::header<false> m_header;
};

class MovedResponseChecker : public Checker
{
public:
	MovedResponseChecker(std::string_view redirect) : m_redirect{redirect} {}

	template <typename Response>
	void operator()(Response&& res) const
	{
		REQUIRE(res.result() == http::status::see_other);
		REQUIRE(res.at(http::field::location) == m_redirect);
		REQUIRE(res.version() == 11);
		set_tested(res);
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
		INFO("response type = " << boost::core::demangled_name(typeid(res)));

		REQUIRE(res.result() == m_status);
		REQUIRE(res.version() == 11);
		set_tested(res);
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
		REQUIRE(check_resource_content(m_file, std::move(res)));
		REQUIRE(res.version() == 11);

		set_tested(res);
	}

private:
	http::status m_status;
	boost::filesystem::path m_file;
};

Authentication create_session(std::string_view username, std::string_view password, const Configuration& cfg)
{
	boost::asio::io_context ioc;
	auto db = redis::connect(ioc, cfg.redis());

	std::promise<Authentication> result;

	Authentication::add_user(username, Password{password}, *db, [&result, db, username, password](std::error_code ec)
	{
		REQUIRE(!ec);

		using namespace std::chrono_literals;
		Authentication::verify_user(username, Password{password}, *db, 60s, [&result, db](std::error_code ec, auto&& auth)
		{
			REQUIRE(!ec);
			REQUIRE(auth.valid());
			result.set_value(auth);
			db->disconnect();
		});
	});
	ioc.run();
	return result.get_future().get();
}

}

TEST_CASE("General server tests", "[normal]")
{
	auto local_json = (current_src / "../../../etc/hearty_rabbit/hearty_rabbit.json").string();

	const char *argv[] = {"hearty_rabbit", "--cfg", local_json.c_str()};
	Configuration cfg{sizeof(argv)/sizeof(argv[1]), argv, nullptr};

	auto session = create_session("testuser", "password", cfg);

	Server server{cfg};
	auto subject = server.start_session();
	REQUIRE(cfg.web_root() == (current_src/"../../../lib").lexically_normal());

	SECTION("Request with empty body")
	{
		EmptyRequest req;
		req.version(11);
		req.method(http::verb::get);

		SECTION("Request index.html success without login")
		{
			GenericStatusChecker checker{http::status::ok};

			req.target("/");
			subject.handle_request(std::move(req), std::ref(checker), {});
			REQUIRE(server.get_io_context().run_for(10s) > 0);
			REQUIRE(checker.tested());
		}

		SECTION("Request logo.svg success without login")
		{
			FileResponseChecker checker{http::status::ok, cfg.web_root() / "static/logo.svg"};

			req.target("/lib/logo.svg");
			subject.handle_request(std::move(req), std::ref(checker), {});
			REQUIRE(checker.tested());
		}

		SECTION("Request hearty_rabbit.js success with login")
		{
			FileResponseChecker checker{http::status::ok, cfg.web_root() / "static/hearty_rabbit.js"};

			req.target("/lib/hearty_rabbit.js");
			req.set(boost::beast::http::field::cookie, session.set_cookie());
			subject.handle_request(std::move(req), std::ref(checker), session);
			REQUIRE(checker.tested());
		}

		SECTION("Request index.html failed without login")
		{
			GenericStatusChecker checker{http::status::not_found};

			req.target("/index.html");
			subject.handle_request(std::move(req), std::ref(checker), {});
			REQUIRE(checker.tested());
		}

		SECTION("Request index.html success with login")
		{
			// TODO: check index.html file content
			GenericStatusChecker checker{http::status::ok};

			req.target("/");
			req.set(boost::beast::http::field::cookie, session.set_cookie());
			subject.handle_request(std::move(req), std::ref(checker), session);
			REQUIRE(server.get_io_context().run_for(10s) > 0);
			REQUIRE(checker.tested());
		}

		SECTION("requesting something not exist")
		{
			GenericStatusChecker checker{http::status::not_found};

			req.target("/something_not_exist.html");
			subject.handle_request(std::move(req), std::ref(checker), {});
			REQUIRE(checker.tested());
		}

		SECTION("Only allow login with POST: GET request is bad request")
		{
			GenericStatusChecker checker{http::status::bad_request};

			req.target("/login");
			subject.handle_request(std::move(req), std::ref(checker), {});
			REQUIRE(checker.tested());
		}

		SECTION("requesting other resources without a session")
		{
			GenericStatusChecker home_page{http::status::ok};
			GenericStatusChecker not_found{http::status::not_found};
			Checker *expected{nullptr};

			SECTION("requests for / will see login page without delay")
			{
				req.target("/");
				subject.handle_request(std::move(req), std::ref(home_page), {});
				REQUIRE(server.get_io_context().run_for(10s) > 0);
				expected = &home_page;
			}
			SECTION("requests to others will get not found")
			{
				req.target("/something");
				subject.handle_request(std::move(req), std::ref(not_found), {});
				expected = &not_found;
			}
			REQUIRE(expected != nullptr);
			REQUIRE(expected->tested());
		}
	}
	SECTION("Request with string body")
	{
		StringRequest req;
		req.version(11);

		SECTION("Login Incorrect")
		{
			MovedResponseChecker login_incorrect{"/lib/login_incorrect.html"};
			MovedResponseChecker invalid_login{"/"};

			req.target("/login");
			req.method(http::verb::post);
			req.body() = "username=user&password=123";

			SECTION("correct content_type")
			{
				req.set(http::field::content_type, "application/x-www-form-urlencoded");
				subject.handle_request(std::move(req), std::ref(login_incorrect), {});
				REQUIRE(server.get_io_context().run_for(10s) > 0);
				REQUIRE(login_incorrect.tested());
				REQUIRE(login_incorrect[http::field::cookie] == "");
			}
			SECTION("without content_type")
			{
				req.erase(http::field::content_type);
				REQUIRE(req[http::field::content_type] == "");
				subject.handle_request(std::move(req), std::ref(invalid_login), {});
				REQUIRE(invalid_login.tested());
				REQUIRE(invalid_login[http::field::cookie] == "");
			}
			SECTION("without incorrect content_type")
			{
				req.insert(http::field::content_type, "text/plain");
				subject.handle_request(std::move(req), std::ref(invalid_login), {});
				REQUIRE(invalid_login.tested());
				REQUIRE(invalid_login[http::field::cookie] == "");
			}
		}
	}

	SECTION("Upload request")
	{
		UploadRequest req;
		req.target("/upload/testuser/testdata");
		req.method(http::verb::put);

		boost::system::error_code bec;
		req.body().open(cfg.blob_path(), bec);
		REQUIRE(bec == boost::system::error_code{});

		std::error_code sec;
		auto testdata = MMap::open(__FILE__, sec);
		REQUIRE(sec == std::error_code{});

		req.body().write(testdata.data(), testdata.size(), bec);
		REQUIRE(bec == boost::system::error_code{});

		SECTION("request with valid session response with 200 created")
		{
			GenericStatusChecker checker{http::status::created};
			req.set(boost::beast::http::field::cookie, session.set_cookie());
			subject.handle_request(std::move(req), std::ref(checker), session);
			REQUIRE(server.get_io_context().run_for(10s) > 0);
			REQUIRE(checker.tested());
			REQUIRE(checker[http::field::location] != "");

			std::string location{checker[http::field::location]};

			INFO("Upload request returned location: " << location);

			EmptyRequest get_blob;
			get_blob.method(http::verb::get);
			get_blob.target(location);

			server.get_io_context().restart();

			SECTION("get back the uploaded blob with valid session")
			{
				get_blob.set(boost::beast::http::field::cookie, session.set_cookie());
				FileResponseChecker blob{http::status::ok, __FILE__};
				subject.handle_request(std::move(get_blob), std::ref(blob), session);
				REQUIRE(server.get_io_context().run_for(3610s) > 0);
				REQUIRE(blob.tested());
			}
			SECTION("get 403 without valid session and delay")
			{
				GenericStatusChecker forbidden{http::status::forbidden};
				subject.handle_request(std::move(get_blob), std::ref(forbidden), {});
				REQUIRE(server.get_io_context().run_for(10s) > 0);
				REQUIRE(forbidden.tested());
			}
		}
		SECTION("request without valid session response with 403 forbidden and delay")
		{
			GenericStatusChecker checker{http::status::forbidden};
			subject.handle_request(std::move(req), std::ref(checker), {});
//			REQUIRE(server.get_io_context().run_for(10s) > 0);
			REQUIRE(checker.tested());
		}
	}
}
