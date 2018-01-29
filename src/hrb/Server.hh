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
#include "net/Redis.hh"

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
		if (req.target().starts_with("/login"))
			return send(set_common_fields(req, on_login(req)));

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
	http::response<http::empty_body> on_login(const Request& req);

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

private:
	static std::string_view resource_mime(const std::string& ext);
	static void drop_privileges();

private:
	const Configuration&    m_cfg;
	boost::asio::io_context m_ioc;

	std::mutex m_redis_mx;
	std::list<redis::Database> m_redis_pool;
};

} // end of namespace
