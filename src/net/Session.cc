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
#include "util/Log.hh"

#include <boost/beast/version.hpp>
#include <boost/beast/http/message.hpp>

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>

#include <iostream>

namespace hrb {

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

Session::Session(
	boost::asio::ip::tcp::socket socket,
	const boost::filesystem::path& doc_root,
	boost::asio::ssl::context *ssl_ctx
) :
	m_socket{std::move(socket)},
	m_strand{m_socket.get_executor()},
	m_doc_root{doc_root}
{
	if (ssl_ctx)
		m_stream.emplace(m_socket, *ssl_ctx);
}

// Start the asynchronous operation
void Session::run()
{
	if (m_stream)
	{
		// Perform the SSL handshake
		m_stream->async_handshake(
			boost::asio::ssl::stream_base::server,
			boost::asio::bind_executor(
				m_strand,
				[self = shared_from_this()](auto ec)
				{ self->on_handshake(ec); }
			)
		);
	}
	else
	{
		do_read();
	}
}

void Session::on_handshake(boost::system::error_code ec)
{
	if (ec)
		LOG(LOG_WARNING, "handshake error: %d (%s)", ec.value(), ec.message());

	do_read();
}

void Session::do_read()
{
	auto&& executor = boost::asio::bind_executor(
		m_strand,
		[self=shared_from_this()](auto ec, auto bytes){self->on_read(ec, bytes);}
	);

	// Read a request
	if (m_stream)
		boost::beast::http::async_read(*m_stream, m_buffer, m_req, std::move(executor));
	else
		boost::beast::http::async_read(m_socket, m_buffer, m_req, std::move(executor));
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
	const boost::filesystem::path& doc_root,
	http::request<Body, http::basic_fields<Allocator>>&& req,
	Send &&send)
{
	LOG(LOG_INFO, "request received %s", req.target().to_string().c_str());
	std::cout << "request " << req.target() << std::endl;

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
		auto path = (doc_root / req.target().to_string()).string();
		LOG(LOG_NOTICE, "requesting path %s", path.c_str());

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
		LOG(LOG_WARNING, "read error: %d (%s)", ec.value(), ec.message());

	// Send the response
	handle_request(m_doc_root, std::move(m_req), [this](auto&& msg)
	{
		// The lifetime of the message has to extend
		// for the duration of the async operation so
		// we use a shared_ptr to manage it.
		auto sp = std::make_shared<std::remove_reference_t<decltype(msg)>>(std::move(msg));

		auto&& executor = boost::asio::bind_executor(
			m_strand,
			[self=this->shared_from_this(), sp](auto error_code, auto bytes_transferred)
			{
				self->on_write(error_code, bytes_transferred, sp->need_eof());
			}
		);

		// Write the response
		if (m_stream)
			boost::beast::http::async_write(*m_stream, *sp, std::move(executor));
		else
			boost::beast::http::async_write(m_socket, *sp, std::move(executor));
	});
}

void Session::on_write(
	boost::system::error_code ec,
	std::size_t,
	bool close)
{
	if (ec)
		LOG(LOG_WARNING, "write error: %d (%s)", ec.value(), ec.message());

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
	if (m_stream)
	{
		m_stream->async_shutdown(
			boost::asio::bind_executor(
				m_strand,
				[self=shared_from_this()](auto ec){self->on_shutdown(ec);}
			)
		);
	}
	else
	{
		m_socket.shutdown(tcp::socket::shutdown_send, ec);
		on_shutdown(ec);
	}
}

void Session::on_shutdown(boost::system::error_code ec)
{
	if (!ec)
		LOG(LOG_WARNING, "shutdown error: %d (%s)", ec.value(), ec.message());

	// At this point the connection is closed gracefully
}

} // end of namespace
