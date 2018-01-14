/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/7/18.
//

#include "Server.hh"

namespace hrb {

Server::Server(const Configuration& cfg) :
	m_cfg{cfg}
{
}

http::response<boost::beast::http::empty_body> Server::redirect(boost::beast::string_view where, unsigned version)
{
	http::response<http::empty_body> res{http::status::moved_permanently, version};
	res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
	res.set(http::field::location, where);
	res.keep_alive(true);
	return res;
}

http::response<http::string_body>
Server::get_blob(http::request<http::string_body> &&req)
{
	return http::response<http::string_body>();
}

http::response<http::string_body> Server::bad_request(const Request& req, boost::beast::string_view why)
{
	http::response<http::string_body> res{http::status::bad_request, req.version()};
	res.set(http::field::content_type, "text/html");
	res.body() = why.to_string();
	res.prepare_payload();
	return set_common_fields(req, res);
}

// Returns a not found response
http::response<http::string_body> Server::not_found(const Request& req, boost::beast::string_view target)
{
	http::response<http::string_body> res{http::status::not_found, req.version()};
	res.set(http::field::content_type, "text/html");
	res.body() = "The resource '" + target.to_string() + "' was not found.";
	res.prepare_payload();
	return set_common_fields(req, res);
}

http::response<http::string_body> Server::server_error(const Request& req, boost::beast::string_view what)
{
	http::response<http::string_body> res{http::status::internal_server_error, req.version()};
	res.set(http::field::content_type, "text/html");
	res.body() = "An error occurred: '" + what.to_string() + "'";
	res.prepare_payload();
	return set_common_fields(req, res);
}

http::response<http::file_body> Server::file_request(const Request& req)
{
	auto path = (m_cfg.web_root() / req.target().to_string()).string();
	Log(LOG_NOTICE, "requesting path %1%", path);

    // Attempt to open the file
    boost::beast::error_code ec;
    http::file_body::value_type file;
    file.open(path.c_str(), boost::beast::file_mode::scan, ec);

	// Handle the case where the file doesn't exist
	if (!ec)
		throw std::system_error(ec);

	auto file_size = file.size();

	http::response<http::file_body> res{
	    std::piecewise_construct,
	    std::make_tuple(std::move(file)),
	    std::make_tuple(http::status::ok, req.version())
	};
	res.set(http::field::content_type, "text/html");
	res.content_length(file_size);
	return set_common_fields(req, res);

}

http::response<http::string_body> Server::hello_world(const Request &req)
{
	std::string mime = "text/html";
	std::string body = "Hello world!";

	// Respond to GET request
	http::response<http::string_body> res{http::status::ok, req.version()};
	res.set(http::field::content_type, mime);
	res.content_length(body.size());
	res.body() = std::move(body);
	return set_common_fields(req, res);

}

} // end of namespace
