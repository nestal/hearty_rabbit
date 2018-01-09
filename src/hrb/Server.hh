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

#include "util/Log.hh"
#include "util/Exception.hh"
#include "util/Configuration.hh"

#include <boost/beast.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/version.hpp>

namespace hrb {

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

class Server
{
public:
	explicit Server(const Configuration& cfg);

	// This function produces an HTTP response for the given
	// request. The type of the response object depends on the
	// contents of the request, so the interface requires the
	// caller to pass a generic lambda for receiving the response.
	template<
		class Body, class Allocator,
		class Send>
	void handle_http(
		const boost::asio::ip::tcp::endpoint& peer,
		http::request<Body, http::basic_fields<Allocator>>&& req,
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
	template<
		class Body, class Allocator,
		class Send>
	void handle_https(
		const boost::asio::ip::tcp::endpoint& peer,
		http::request<Body, http::basic_fields<Allocator>>&& req,
		Send&& send
	)
	{
		Log(LOG_INFO, "request %1% from %2%", req.target(), peer);

		// Make sure we can handle the method
		if (req.method() != http::verb::get &&
		    req.method() != http::verb::head)
			return send(bad_request(req, "Unknown HTTP-method"));

		// Request path must be absolute and not contain "..".
		if (req.target().empty() ||
		    req.target()[0] != '/' ||
		    req.target().find("..") != boost::beast::string_view::npos)
			return send(bad_request(req, "Illegal request-target"));

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

		if (req.target() == "/index.html")
		{
			auto path = (m_cfg.web_root() / req.target().to_string()).string();
			Log(LOG_NOTICE, "requesting path %1%", path);

		    // Attempt to open the file
		    boost::beast::error_code ec;
		    http::file_body::value_type file;
		    file.open(path.c_str(), boost::beast::file_mode::scan, ec);

			// Handle the case where the file doesn't exist
			if(ec == boost::system::errc::no_such_file_or_directory)
				return send(not_found(req, req.target()));

			// Handle an unknown error
			if(ec)
				return send(server_error(req, ec.message()));
			    // Respond to GET request

			auto file_size = file.size();

			http::response<http::file_body> res{
			    std::piecewise_construct,
			    std::make_tuple(std::move(file)),
			    std::make_tuple(http::status::ok, req.version())
			};
			res.set(http::field::content_type, mime);
			res.content_length(file_size);
			return send(set_common_fields(req, res));
		}

		// Respond to GET request
		http::response<http::string_body> res{http::status::ok, req.version()};
		res.set(http::field::content_type, mime);
		res.content_length(body.size());
		res.body() = std::move(body);
		return send(set_common_fields(req, res));
	}

private:
	static http::response<http::empty_body> redirect(boost::beast::string_view where, unsigned version);

	template<class ReqBody, class ReqAllocator, class ResBody, class ResAllocator>
	static auto&& set_common_fields(
		const http::request<ReqBody, http::basic_fields<ReqAllocator>>& req,
		http::response<ResBody, http::basic_fields<ResAllocator>>& res
	)
	{
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
		res.keep_alive(req.keep_alive());
		return std::move(res);
	}

	// Returns a bad request response
	template<class Body, class Allocator>
	static http::response<http::string_body> bad_request(
		const http::request<Body, http::basic_fields<Allocator>>& req,
		boost::beast::string_view why
	)
	{
		http::response<http::string_body> res{http::status::bad_request, req.version()};
		res.set(http::field::content_type, "text/html");
		res.body() = why.to_string();
		res.prepare_payload();
		return set_common_fields(req, res);
	}

	// Returns a not found response
	template<class Body, class Allocator>
	static http::response<http::string_body> not_found(
		const http::request<Body, http::basic_fields<Allocator>>& req,
		boost::beast::string_view target
	)
	{
		http::response<http::string_body> res{http::status::not_found, req.version()};
		res.set(http::field::content_type, "text/html");
		res.body() = "The resource '" + target.to_string() + "' was not found.";
		res.prepare_payload();
		return set_common_fields(req, res);
	}

	template<class Body, class Allocator>
	static http::response<http::string_body> server_error(
		const http::request<Body, http::basic_fields<Allocator>>& req,
		boost::beast::string_view what
	)
	{
		http::response<http::string_body> res{http::status::internal_server_error, req.version()};
		res.set(http::field::content_type, "text/html");
		res.body() = "An error occurred: '" + what.to_string() + "'";
		res.prepare_payload();
		return set_common_fields(req, res);
	}

private:
	const Configuration& m_cfg;
};

} // end of namespace
