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

namespace hrb {

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

Session::Session(
	boost::asio::ip::tcp::socket socket,
	Server& server,
	boost::asio::ssl::context *ssl_ctx
) :
	m_socket{std::move(socket)},
	m_strand{m_socket.get_executor()},
	m_server{server}
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

void Session::on_read(boost::system::error_code ec, std::size_t)
{
	// This means they closed the connection
	if (ec == boost::beast::http::error::end_of_stream)
		return do_close();

	if (ec)
		LOG(LOG_WARNING, "read error: %d (%s)", ec.value(), ec.message());

	// Send the response
	auto sender = [this](auto&& msg)
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
	};

	if (m_stream)
		m_server.handle_https(m_socket.remote_endpoint(ec), std::move(m_req), std::move(sender));
	else
		m_server.handle_http(m_socket.remote_endpoint(ec), std::move(m_req), std::move(sender));

	if (ec)
		LOG(LOG_WARNING, "remote_endpoint(): %d (%s)", ec.value(), ec.message());
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
	if (ec)
		LOG(LOG_WARNING, "shutdown error: %d (%s)", ec.value(), ec.message());

	// At this point the connection is closed gracefully
}

} // end of namespace
