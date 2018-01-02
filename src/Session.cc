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

#include <systemd/sd-journal.h>

namespace hrb {

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

Session::Session(
	boost::asio::ip::tcp::socket socket,
	std::string const &doc_root)
	: m_socket(std::move(socket)), m_strand(m_socket.get_executor()), m_doc_root(doc_root)
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
	boost::beast::http::async_read(m_socket, m_buffer, m_req,
		boost::asio::bind_executor(
			m_strand,
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
	http::request<Body, http::basic_fields<Allocator>>&& req,
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

	if (req.target() == "/index.html")
	{
		auto path = doc_root.to_string() + req.target().to_string();
		sd_journal_print(LOG_NOTICE, "requesting path %s", path.c_str());

	    // Attempt to open the file
	    boost::beast::error_code ec;
	    http::file_body::value_type file;
	    file.open(path.c_str(), boost::beast::file_mode::scan, ec);

		// Handle the case where the file doesn't exist
		if(ec == boost::system::errc::no_such_file_or_directory)
			return send(not_found(req.target()));

		// Handle an unknown error
		if(ec)
			return send(server_error(ec.message()));
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

void Session::on_read(boost::system::error_code ec, std::size_t)
{
	// This means they closed the connection
	if (ec == boost::beast::http::error::end_of_stream)
		return do_close();

	if (ec)
		sd_journal_print(LOG_WARNING, "read error: %d (%s)", ec.value(), ec.message());

	// Send the response
	handle_request(m_doc_root, std::move(m_req), [this](auto&& msg)
	{
		// The lifetime of the message has to extend
		// for the duration of the async operation so
		// we use a shared_ptr to manage it.
		auto sp = std::make_shared<std::remove_reference_t<decltype(msg)>>(std::move(msg));

		// Write the response
		boost::beast::http::async_write(
			m_socket,
			*sp,
			boost::asio::bind_executor(
				m_strand,
				[self=this->shared_from_this(), sp](auto error_code, auto bytes_transferred)
				{
					self->on_write(error_code, bytes_transferred, sp->need_eof());
				}
			)
		);
	});
}

void Session::on_write(
	boost::system::error_code ec,
	std::size_t,
	bool close)
{
	if (ec)
		sd_journal_print(LOG_WARNING, "write error: %d (%s)", ec.value(), ec.message());

	if (close)
	{
		// This means we should close the connection, usually because
		// the response indicated the "Connection: close" semantic.
		return do_close();
	}

	// Read another request
	do_read();
}

void Session::do_close()
{
	// Send a TCP shutdown
	boost::system::error_code ec;
	m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);

	// At this point the connection is closed gracefully
}

} // end of namespace
