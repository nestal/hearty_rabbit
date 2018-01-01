/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/1/18.
//

#include "Session.hh"
#include <boost/beast/version.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

namespace hrb {

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

Session::Session(
	boost::asio::ip::tcp::socket socket,
	std::string const &doc_root)
	: socket_(std::move(socket)), strand_(socket_.get_executor()), doc_root_(doc_root), lambda_(*this)
{
}

// Start the asynchronous operation
void Session::run()
{
	do_read();
}

void Session::do_read()
{
	// Read a request
	boost::beast::http::async_read(socket_, buffer_, req_,
		boost::asio::bind_executor(
			strand_,
			std::bind(
				&Session::on_read,
				shared_from_this(),
				std::placeholders::_1,
				std::placeholders::_2
			)
		)
	);
}

// This function produces an HTTP response for the given
// request. The type of the response object depends on the
// contents of the request, so the interface requires the
// caller to pass a generic lambda for receiving the response.
template<
	class Body, class Allocator,
	class Send>
void
handle_request(
	boost::beast::string_view doc_root,
	http::request<Body, http::basic_fields<Allocator>> &&req,
	Send &&send)
{
	// Returns a bad request response
	auto const bad_request =
		[&req](boost::beast::string_view why)
		{
			http::response<http::string_body> res{http::status::bad_request, req.version()};
			res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
			res.set(http::field::content_type, "text/html");
			res.keep_alive(req.keep_alive());
			res.body() = why.to_string();
			res.prepare_payload();
			return res;
		};

	// Returns a not found response
	auto const not_found =
		[&req](boost::beast::string_view target)
		{
			http::response<http::string_body> res{http::status::not_found, req.version()};
			res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
			res.set(http::field::content_type, "text/html");
			res.keep_alive(req.keep_alive());
			res.body() = "The resource '" + target.to_string() + "' was not found.";
			res.prepare_payload();
			return res;
		};

	// Returns a server error response
	auto const server_error =
		[&req](boost::beast::string_view what)
		{
			http::response<http::string_body> res{http::status::internal_server_error, req.version()};
			res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
			res.set(http::field::content_type, "text/html");
			res.keep_alive(req.keep_alive());
			res.body() = "An error occurred: '" + what.to_string() + "'";
			res.prepare_payload();
			return res;
		};

	// Make sure we can handle the method
	if (req.method() != http::verb::get &&
	    req.method() != http::verb::head)
		return send(bad_request("Unknown HTTP-method"));

	// Request path must be absolute and not contain "..".
	if (req.target().empty() ||
	    req.target()[0] != '/' ||
	    req.target().find("..") != boost::beast::string_view::npos)
		return send(bad_request("Illegal request-target"));

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

	// Respond to GET request
	http::response<http::string_body> res{http::status::ok, req.version()};
	res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
	res.set(http::field::content_type, mime);
	res.content_length(body.size());
	res.body() = std::move(body);
	res.keep_alive(req.keep_alive());
	return send(std::move(res));
}

void Session::on_read(
	boost::system::error_code ec,
	std::size_t bytes_transferred)
{
	boost::ignore_unused(bytes_transferred);

	// This means they closed the connection
	if (ec == boost::beast::http::error::end_of_stream)
		return do_close();

	if (ec)
		throw std::system_error(ec);

	// Send the response
	handle_request(doc_root_, std::move(req_), lambda_);
}

void Session::on_write(
	boost::system::error_code ec,
	std::size_t bytes_transferred,
	bool close)
{
	boost::ignore_unused(bytes_transferred);

	if (ec)
		throw std::system_error(ec);

	if (close)
	{
		// This means we should close the connection, usually because
		// the response indicated the "Connection: close" semantic.
		return do_close();
	}

	// We're done with the response so delete it
	res_ = nullptr;

	// Read another request
	do_read();
}

void Session::do_close()
{
	// Send a TCP shutdown
	boost::system::error_code ec;
	socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);

	// At this point the connection is closed gracefully
}

} // end of namespace
