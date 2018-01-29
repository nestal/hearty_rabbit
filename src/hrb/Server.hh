/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/7/18.
//

#pragma once

#include "Request.hh"

#include "crypto/Password.hh"
#include "crypto/Authenication.hh"
#include "net/Redis.hh"
#include "util/Configuration.hh"
#include "util/Escape.hh"
#include "util/Log.hh"

#include <boost/filesystem/path.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/file_body.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/io_context.hpp>

#include <mutex>
#include <list>

namespace hrb {

class Configuration;
class Password;

class Server
{
public:
	explicit Server(const Configuration& cfg);

	http::response<http::empty_body> redirect_http(const Request& req);

	// This function produces an HTTP response for the given
	// request. The type of the response object depends on the
	// contents of the request, so the interface requires the
	// caller to pass a generic lambda for receiving the response.
	template<class Send>
	void handle_https(Request&& req, Send&& send)
	{
		if (req.target() == "/login" && req.method() == http::verb::post)
			return on_login(req, std::forward<Send>(send));

		if (req.target().starts_with("/blob"))
			return send(set_common_fields(req, get_blob(req)));

		if (req.target().starts_with("/dir"))
			return send(set_common_fields(req, get_dir(req)));

		return send(file_request(req));
	}

	http::response<http::file_body>   file_request(const Request& req);
	http::response<http::string_body> bad_request(const Request& req, boost::beast::string_view why);
	http::response<http::string_body> not_found(const Request& req, boost::beast::string_view target);
	http::response<http::string_body> server_error(const Request& req, boost::beast::string_view what);
	http::response<http::empty_body> redirect(boost::beast::string_view where, unsigned version);

	http::response<http::string_body> get_blob(const Request& req);
	http::response<http::string_body> get_dir(const Request& req);

	template<class Body, class Allocator>
	static auto&& set_common_fields(
		const Request& req,
		http::response<Body, http::basic_fields<Allocator>>&& res
	)
	{
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
		res.keep_alive(req.keep_alive());
		return std::move(res);
	}

	void run();
	boost::asio::io_context& get_io_context();

	// Administrative commands
	void add_user(std::string_view username, Password&& password, std::function<void(std::error_code)> complete);

private:
	static std::string_view resource_mime(const std::string& ext);
	static void drop_privileges();

	template <typename Send>
	void on_login(const Request& req, Send&& send)
	{
		auto&& body = req.body();
		if (req[http::field::content_type] == "application/x-www-form-urlencoded")
		{
			std::string_view username;
			Password password;
			visit_form_string({body}, [&username, &password](auto name, auto val)
			{
				if (name == "username")
					username = val;
				else if (name == "password")
					password = Password{val};
			});

			auto db = std::make_shared<redis::Database>(m_ioc, m_cfg.redis_host(), m_cfg.redis_port());
			verify_user(
				username,
				std::move(password),
				*db,
				[db, version=req.version(), send=std::forward<Send>(send), this, keep_alive=req.keep_alive()](std::error_code ec)
				{
					Log(LOG_INFO, "login result: %1% %2%", ec, ec.message());
					db->disconnect();

					auto&& res = redirect(ec ? "/login_incorrect.html" : "/login_correct.html", version);
					res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
					res.keep_alive(keep_alive);
					send(std::move(res));
				}
			);
		}
		else
			send(set_common_fields(req, redirect("/login.html", req.version())));
	}
private:
	const Configuration&    m_cfg;
	boost::asio::io_context m_ioc;

	std::mutex m_redis_mx;
	std::list<redis::Database> m_redis_pool;
};

} // end of namespace
