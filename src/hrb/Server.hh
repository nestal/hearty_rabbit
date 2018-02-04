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
#include "DatabasePool.hh"

#include "crypto/Authenication.hh"

#include <boost/filesystem/path.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/file_body.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/io_context.hpp>

#include <system_error>

namespace hrb {

// URL prefixes
namespace url {
const boost::string_view login{"/login"};
const boost::string_view logout{"/logout"};
const boost::string_view blob{"/blob"};
const boost::string_view dir{"/dir"};
}

class Configuration;
class Password;
class BlobObject;

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
		// Obviously "/login" always allow anonymous access, otherwise no one can login.
		if (req.target() == url::login && req.method() == http::verb::post)
			return on_login(req, std::forward<Send>(send));

		// Only index.html require login
		if (allow_anonymous(req.target()))
		{
			auto opt_res = file_request(req);
			if (opt_res)
				return send(std::move(*opt_res));
		}

		auto cookie = req[http::field::cookie];
		auto session = parse_cookie({cookie.data(), cookie.size()});
		if (session)
		{
			auto db = m_db.alloc(m_ioc);
			verify_session(
				*session,
				*db,
				[
					this,
					db,
					req=std::move(req),
					send=std::forward<Send>(send)
				](std::error_code ec, std::string_view user) mutable
				{
					if (ec)
						return send(set_common_fields(req, redirect("/login.html", req.version())));

					if (req.target().starts_with(url::blob))
						return get_blob(req, std::forward<decltype(send)>(send));

					if (req.target().starts_with(url::dir))
						return send(set_common_fields(req, get_dir(req)));

					if (req.target().starts_with(url::logout))
						return on_logout(req, std::forward<Send>(send));

					auto opt_res = file_request(req);
					if (opt_res)
						return send(std::move(*opt_res));

					return send(set_common_fields(req, redirect("/login.html", req.version())));
				}
			);
		}
		else
			return send(set_common_fields(req, redirect("/login.html", req.version())));
	}

	std::optional<http::response<http::file_body>> file_request(const Request& req);
	http::response<http::string_body> bad_request(const Request& req, boost::beast::string_view why);
	http::response<http::string_body> not_found(const Request& req, boost::beast::string_view target);
	http::response<http::string_body> server_error(const Request& req, boost::beast::string_view what);
	http::response<http::empty_body> redirect(boost::beast::string_view where, unsigned version);

	template<class Body, class Allocator>
	static auto&& set_common_fields(
		const Request& req,
		http::response<Body, http::basic_fields<Allocator>>&& res
	)
	{
		return set_common_fields(req.keep_alive(), std::move(res));
	}

	template<class Body, class Allocator>
	static auto&& set_common_fields(
		bool keep_alive,
		http::response<Body, http::basic_fields<Allocator>>&& res
	)
	{
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
		res.keep_alive(keep_alive);
		return std::move(res);
	}

	void disconnect_db();
	void run();
	boost::asio::io_context& get_io_context();

	// Administrative commands
	void add_user(std::string_view username, Password&& password, std::function<void(std::error_code)> complete);
	void add_blob(const boost::filesystem::path& path, std::function<void(BlobObject&, std::error_code)> complete);

private:
	static std::string_view resource_mime(const std::string& ext);
	static void drop_privileges();

	void on_login(const Request& req, std::function<void(http::response<http::empty_body>&&)>&& send);
	void on_logout(const Request& req, std::function<void(http::response<http::empty_body>&&)>&& send);
	void get_blob(const Request& req, std::function<void(http::response<http::string_body>&&)>&& send);
	http::response<http::string_body> get_dir(const Request& req);
	static bool allow_anonymous(boost::string_view target);

private:
	const Configuration&    m_cfg;
	boost::asio::io_context m_ioc;

	DatabasePool    m_db;
};

} // end of namespace
