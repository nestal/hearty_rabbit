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
	Server(const boost::filesystem::path& doc_root);


	// This function produces an HTTP response for the given
	// request. The type of the response object depends on the
	// contents of the request, so the interface requires the
	// caller to pass a generic lambda for receiving the response.
	template<
		class Body, class Allocator,
		class Send>
	void handle_request(
		const boost::asio::ip::tcp::endpoint& peer,
		http::request<Body, http::basic_fields<Allocator>>&& req,
		Send &&send
	)
	{
		std::ostringstream ss;
		ss << peer;
		LOG(LOG_INFO, "request %s from %s", req.target().to_string().c_str(), ss.str().c_str());

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
			res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
			res.set(http::field::content_type, mime);
			res.content_length(body.size());
			res.keep_alive(req.keep_alive());
			return send(std::move(res));
		}

		if (req.target() == "/index.html")
		{
			auto path = (m_doc_root / req.target().to_string()).string();
			LOG(LOG_NOTICE, "requesting path %s", path.c_str());

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
			res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
			res.set(http::field::content_type, mime);
			res.content_length(file_size);
			res.keep_alive(req.keep_alive());
			return send(std::move(res));
		}

		// Respond to GET request
		http::response<http::string_body> res{http::status::ok, req.version()};
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(http::field::content_type, mime);
		res.content_length(body.size());
		res.body() = std::move(body);
		res.keep_alive(req.keep_alive());
		return send(std::move(res));
	}

	static http::response<http::empty_body> redirect(boost::beast::string_view where, unsigned version);

private:
	// Returns a bad request response
	template<class Body, class Allocator>
	static http::response<http::string_body> bad_request(
		const http::request<Body, http::basic_fields<Allocator>>& req,
		boost::beast::string_view why
	)
	{
		http::response<http::string_body> res{http::status::bad_request, req.version()};
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(http::field::content_type, "text/html");
		res.keep_alive(req.keep_alive());
		res.body() = why.to_string();
		res.prepare_payload();
		return res;
	}

	// Returns a not found response
	template<class Body, class Allocator>
	static http::response<http::string_body> not_found(
		const http::request<Body, http::basic_fields<Allocator>>& req,
		boost::beast::string_view target
	)
	{
		http::response<http::string_body> res{http::status::not_found, req.version()};
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(http::field::content_type, "text/html");
		res.keep_alive(req.keep_alive());
		res.body() = "The resource '" + target.to_string() + "' was not found.";
		res.prepare_payload();
		return res;
	}

	template<class Body, class Allocator>
	static http::response<http::string_body> server_error(
		const http::request<Body, http::basic_fields<Allocator>>& req,
		boost::beast::string_view what
	)
	{
		http::response<http::string_body> res{http::status::internal_server_error, req.version()};
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(http::field::content_type, "text/html");
		res.keep_alive(req.keep_alive());
		res.body() = "An error occurred: '" + what.to_string() + "'";
		res.prepare_payload();
		return res;
	}

private:
	boost::filesystem::path m_doc_root;
};

} // end of namespace
