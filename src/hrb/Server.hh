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

#include "util/Log.hh"
#include "util/Exception.hh"
#include "util/Configuration.hh"

#include <boost/filesystem/path.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/file_body.hpp>
#include <boost/beast/version.hpp>

namespace hrb {

class Server
{
public:
	explicit Server(const Configuration& cfg);

	// This function produces an HTTP response for the given
	// request. The type of the response object depends on the
	// contents of the request, so the interface requires the
	// caller to pass a generic lambda for receiving the response.
	template<class Send>
	void handle_http(
		const boost::asio::ip::tcp::endpoint& peer,
		http::request<http::string_body>&& req,
		Send&& send
	)
	{
		using namespace std::literals;
		static const auto https_host = "https://" + m_cfg.server_name()
			+ (m_cfg.listen_https().port() == 443 ? ""s : (":"s + std::to_string(m_cfg.listen_https().port())));

		auto&& dest = https_host + req.target().to_string();
		Log(LOG_INFO, "redirecting HTTP request %1% to host %2%", req.target(), dest);

		send(Server::redirect(dest, req.version()));
	}

	// This function produces an HTTP response for the given
	// request. The type of the response object depends on the
	// contents of the request, so the interface requires the
	// caller to pass a generic lambda for receiving the response.
	template<class Send>
	void handle_https(
		const boost::asio::ip::tcp::endpoint& peer,
		http::request<http::string_body>&& req,
		Send&& send
	)
	{
		std::string mime = "text/html";
		std::string body = "Hello world!";

		// Respond to HEAD request
		if (req.method() == http::verb::head)
		{
			http::response<http::empty_body> res{http::status::ok, req.version()};
			res.set(http::field::content_type, mime);
			res.content_length(body.size());
			return send(set_common_fields(req, res));
		}

//		if (req.target().start_with("/blob"))
//			return send(set_common_fields(req, get_blob(req)));

		else if (req.target() == "/index.html")
			return send(file_request(req));

		return send(hello_world(req));
	}

	http::response<http::file_body> file_request(const Request& req);
	http::response<http::string_body> hello_world(const Request& req);

	// Returns a bad request response
	http::response<http::string_body> bad_request(const Request& req, boost::beast::string_view why);

	// Returns a not found response
	http::response<http::string_body> not_found(const Request& req, boost::beast::string_view target);

	http::response<http::string_body> server_error(const Request& req, boost::beast::string_view what);

	http::response<http::empty_body> redirect(boost::beast::string_view where, unsigned version);

	http::response<http::string_body> get_blob(http::request<http::string_body>&& req);

	template<class Body, class Allocator>
	static auto&& set_common_fields(
		const Request& req,
		http::response<Body, http::basic_fields<Allocator>>& res
	)
	{
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
		res.keep_alive(req.keep_alive());
		return std::move(res);
	}

private:
	const Configuration& m_cfg;
};

} // end of namespace
