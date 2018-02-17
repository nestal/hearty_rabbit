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
#include "hrb/Server.hh"

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <iostream>

namespace hrb {

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

Session::Session(
	boost::asio::ip::tcp::socket socket,
	Server& server,
	boost::asio::ssl::context *ssl_ctx,
	std::size_t nth
) :
	m_socket{std::move(socket)},
	m_strand{m_socket.get_executor()},
	m_server{server},
	m_nth_session{nth}
{
	// TLS is optional for this class
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
				[self = shared_from_this()](auto ec){self->on_handshake(ec);}
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
		Log(LOG_WARNING, "handshake error: %1%", ec);

	do_read();
}

void Session::do_read()
{
	auto&& executor = boost::asio::bind_executor(
		m_strand,
		[self=shared_from_this()](auto ec, auto bytes) {self->on_read_header(ec, bytes);}
	);

	std::cout << "do read... reuse parser? " << std::endl;
	m_parser.emplace();

	// Read a request
	if (m_stream)
		async_read_header(*m_stream, m_buffer, *m_parser, std::move(executor));
	else
		async_read_header(m_socket, m_buffer, *m_parser, std::move(executor));
}

void Session::on_read_header(boost::system::error_code ec, std::size_t bytes_transferred)
{
	if (ec)
	{
		std::cout << "on_read_header(): error " << ec.message() << " " << bytes_transferred << std::endl;
		on_read(ec, bytes_transferred);
	}
	else
	{
		auto&& header = m_parser->get();

		auto target = header.target();
		std::cout << "on_read_header(): requesting " << target << " " << ec.message() << " " << bytes_transferred
			<< "parser: " << m_parser->is_header_done() << std::endl;

		for (auto&& field: header)
			std::cout << "field = " << field.name() << " (" << field.name_string() << ") " << field.value() << std::endl;

		m_body.emplace<0>(std::move(*m_parser));

		auto&& executor = boost::asio::bind_executor(
			m_strand,
			[self = shared_from_this()](auto ec, auto bytes)
			{ self->on_read(ec, bytes); }
		);

		// Read a request
		if (m_stream)
			async_read(*m_stream, m_buffer, std::get<0>(m_body), std::move(executor));
		else
			async_read(m_socket, m_buffer, std::get<0>(m_body), std::move(executor));
	}
}

// This function produces an HTTP response for the given
// request. The type of the response object depends on the
// contents of the request, so the interface requires the
// caller to pass a generic lambda for receiving the response.
template<class Send>
void Session::handle_https(Request&& req, Send&& send)
{
	try
	{
		boost::system::error_code ec;
		auto endpoint = m_socket.remote_endpoint(ec);
		if (ec)
			Log(LOG_WARNING, "remote_endpoint() error: %1% %2%", ec, ec.message());

		Log(
			LOG_INFO,
			"%1%:%2% request %3% from %4% (version %5%) %6% bytes",
			m_nth_session,
			m_nth_transaction,
			req.target(),
			endpoint,
			req.version(),
			req[http::field::content_length]
		);

		// Make sure we can handle the method
		if (req.method() != http::verb::get  &&
		    req.method() != http::verb::post &&
		    req.method() != http::verb::put &&
			req.method() != http::verb::head)
			return send(m_server.bad_request("Unknown HTTP-method", req.version()));

		// Request path must be absolute and not contain "..".
		if (req.target().empty() ||
		    req.target()[0] != '/' ||
		    req.target().find("..") != boost::beast::string_view::npos)
			return send(m_server.bad_request("Illegal request-target", req.version()));

		return m_server.handle_https(std::move(req), std::move(send));
	}
	catch (std::system_error& e)
	{
		auto ec = e.code();
		// Handle the case where the file doesn't exist
		if( ec == std::errc::no_such_file_or_directory)
			return send(m_server.not_found(req));

		// Handle an unknown error
		if(ec)
			return send(m_server.server_error(req, ec.message()));
	}
}


void Session::on_read(boost::system::error_code ec, std::size_t)
{
	// This means they closed the connection
	if (ec == boost::beast::http::error::end_of_stream)
		return do_close();

	// function that senders the response message
	auto sender = [this, self=shared_from_this()](auto&& response, bool keep_alive)
	{

		// The lifetime of the message has to extend
		// for the duration of the async operation so
		// we use a shared_ptr to manage it.
		auto sp = std::make_shared<std::remove_reference_t<decltype(response)>>(std::move(response));
		sp->set(http::field::server, BOOST_BEAST_VERSION_STRING);
		sp->keep_alive(keep_alive);

		auto&& callback = boost::asio::bind_executor(
			m_strand,
			[self, sp](auto&& ec, auto bytes)
			{ self->on_write(ec, bytes, sp->need_eof()); }
		);

		// Write the response
		if (m_stream)
			async_write(*m_stream, *sp, std::move(callback));
		else
			async_write(m_socket, *sp, std::move(callback));
	};

	if (ec)
	{
		Log(LOG_WARNING, "read error: %1% (%2%)", ec, ec.message());
		sender(m_server.bad_request(ec.message(), 11), false);
	}
	else
	{
		auto req = std::get<0>(m_body).release();
		if (m_stream)
			handle_https(std::move(req), [sender = std::move(sender), keep_alive=req.keep_alive()](auto&& response)
			{
				sender(std::move(response), keep_alive);
			});
		else
			sender(m_server.redirect_http(req), req.keep_alive() );
	}
	m_nth_transaction++;
}

void Session::on_write(
	boost::system::error_code ec,
	std::size_t,
	bool close)
{
	if (ec)
		Log(LOG_WARNING, "write error: %1%", ec);

	if (close)
	{
		// This means we should close the connection, usually because
		// the response indicated the "Connection: close" semantic.
		return do_close();
	}

	// Read another request
	if (!ec)
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
	if (ec)
		Log(LOG_WARNING, "shutdown error: %1% (%2%)", ec, ec.message());

	// At this point the connection is closed gracefully
}

} // end of namespace
