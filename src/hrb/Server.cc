/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/7/18.
//

#include <iostream>
#include "Server.hh"
#include "WebResources.hh"

#include "util/Configuration.hh"
#include "util/Escape.hh"

#include "util/Log.hh"
#include "util/Exception.hh"

namespace hrb {

Server::Server(const Configuration& cfg) :
	m_cfg{cfg}
{
}

http::response<http::empty_body> Server::redirect(boost::beast::string_view where, unsigned version)
{
	http::response<http::empty_body> res{http::status::moved_permanently, version};
	res.set(http::field::location, where);
	return res;
}

http::response<http::string_body> Server::get_blob(const Request& req)
{
	return http::response<http::string_body>();
}

http::response<http::string_body> Server::get_dir(const Request& req)
{
	return http::response<http::string_body>();
}

http::response<http::string_body> Server::bad_request(const Request& req, boost::beast::string_view why)
{
	http::response<http::string_body> res{http::status::bad_request, req.version()};
	res.set(http::field::content_type, "text/html");
	res.body() = why.to_string();
	res.prepare_payload();
	return set_common_fields(req, std::move(res));
}

// Returns a not found response
http::response<http::string_body> Server::not_found(const Request& req, boost::beast::string_view target)
{
	http::response<http::string_body> res{http::status::not_found, req.version()};
	res.set(http::field::content_type, "text/html");
	res.body() = "The resource '" + target.to_string() + "' was not found.";
	res.prepare_payload();
	return set_common_fields(req, std::move(res));
}

http::response<http::string_body> Server::server_error(const Request& req, boost::beast::string_view what)
{
	http::response<http::string_body> res{http::status::internal_server_error, req.version()};
	res.set(http::field::content_type, "text/html");
	res.body() = "An error occurred: '" + what.to_string() + "'";
	res.prepare_payload();
	return set_common_fields(req, std::move(res));
}

http::response<http::file_body> Server::file_request(const Request& req)
{
	Log(LOG_NOTICE, "requesting path %1%", req.target());

	auto filepath = req.target();
	filepath.remove_prefix(1);

	// TODO: use redirect instead
	if (web_resources.find(filepath.to_string()) == web_resources.end())
		filepath = "login.html";

	auto path = m_cfg.web_root() / filepath.to_string();
	Log(LOG_NOTICE, "reading from %1%", path);

	// Attempt to open the file
	boost::beast::error_code ec;
	http::file_body::value_type file;
	file.open(path.string().c_str(), boost::beast::file_mode::scan, ec);

	// Handle the case where the file doesn't exist
	if (ec)
		throw std::system_error(ec);

	auto file_size = file.size();

	http::response<http::file_body> res{
		std::piecewise_construct,
		std::make_tuple(std::move(file)),
		std::make_tuple(http::status::ok, req.version())
	};
	res.set(http::field::content_type, resource_mime(path.extension().string()));
	res.content_length(file_size);
	return set_common_fields(req, std::move(res));
}

http::response<http::empty_body> Server::redirect_http(const Request &req)
{
	using namespace std::literals;
	static const auto https_host = "https://" + m_cfg.server_name()
		+ (m_cfg.listen_https().port() == 443 ? ""s : (":"s + std::to_string(m_cfg.listen_https().port())));

	auto&& dest = https_host + req.target().to_string();
	Log(LOG_INFO, "redirecting HTTP request %1% to host %2%", req.target(), dest);

	return set_common_fields(req, redirect(dest, req.version()));
}

std::string_view Server::resource_mime(const std::string& ext)
{
	// don't expect a big list
	     if (ext == ".html")    return "text/html";
	else if (ext == ".css")     return "text/css";
	else if (ext == ".svg")     return "image/svg+xml";
	else                        return "application/octet-stream";
}

http::response<boost::beast::http::empty_body> Server::on_login(const Request& req)
{
	auto&& body = req.body();
	if (req.at("content-type") == "application/x-www-form-urlencoded")
	{
		visit_form_string({body}, [](auto name, auto val)
		{
			std::cout << "field " << name << ": " << val << std::endl;
		});
	}

	std::cout << body << " " << req.at("content-type") << std::endl;
	return set_common_fields(req, redirect("/index.html", req.version()));
}

} // end of namespace
