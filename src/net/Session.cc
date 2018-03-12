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
#include "hrb/Server.ipp"
#include "hrb/Ownership.ipp"

#include <boost/asio/bind_executor.hpp>
#include <iostream>
#include <util/Error.hh>

namespace hrb {

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

Session::Session(
	Server& server,
	boost::asio::ip::tcp::socket socket,
	boost::asio::ssl::context&  ssl_ctx,
	std::size_t nth
) :
	m_socket{std::move(socket)},
	m_stream{m_socket, ssl_ctx},
	m_strand{m_socket.get_executor()},
	m_server{server},
	m_nth_session{nth}
{
}

// Start the asynchronous operation
void Session::run()
{
	// Perform the SSL handshake
	m_stream.async_handshake(
		boost::asio::ssl::stream_base::server,
		boost::asio::bind_executor(
			m_strand,
			[self = shared_from_this()](auto ec){self->on_handshake(ec);}
		)
	);
}

void Session::on_handshake(boost::system::error_code ec)
{
	if (ec)
		Log(LOG_WARNING, "handshake error: %1%", ec);

	do_read();
}

void Session::do_read()
{
	// Destroy and re-construct the parser for a new HTTP transaction
	m_parser.emplace();

	m_parser->body_limit(m_server.upload_limit());

	// Read the header of a request
	async_read_header(m_stream, m_buffer, *m_parser, boost::asio::bind_executor(
		m_strand,
		[self=shared_from_this()](auto ec, auto bytes) {self->on_read_header(ec, bytes);}
	));
}

void Session::on_read_header(boost::system::error_code ec, std::size_t bytes_transferred)
{
	if (ec)
		handle_read_error(__PRETTY_FUNCTION__, {ec.value(), ec.category()});
	else
	{
		// Get the HTTP header from the partially parsed request message from the parser.
		// The body of the request message has not parsed yet.
		auto&& header = m_parser->get();
		m_keep_alive = header.keep_alive();

		m_server.on_request_header(header, *m_parser, m_body, [self=shared_from_this(), this](const Authentication& auth)
		{
			// Call async_read() using the chosen parser to read and parse the request body.
			std::visit([&self, this, &auth](auto&& parser)
			{
				async_read(m_stream, m_buffer, parser, boost::asio::bind_executor(
					m_strand,
					[self, auth](auto ec, auto bytes){self->on_read(ec, bytes, auth);}
				));
			}, m_body);
		});
	}
}


void Session::on_read(boost::system::error_code ec, std::size_t, const Authentication& auth)
{
	// This means they closed the connection
	if (ec)
		return handle_read_error(__PRETTY_FUNCTION__, ec);
	else
	{
		std::visit([self=shared_from_this(), this, &auth](auto&& parser)
		{
			auto req = parser.release();
			if (validate_request(req))
			{
				m_server.handle_https(std::move(req), [self](auto&& response)
				{
					self->send_response(std::forward<decltype(response)>(response));
				}, auth);
			}
		}, m_body);
	}
	m_nth_transaction++;
}

// This function produces an HTTP response for the given
// request. The type of the response object depends on the
// contents of the request, so the interface requires the
// caller to pass a generic lambda for receiving the response.
template<class Request>
bool Session::validate_request(const Request& req)
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
		req.method() != http::verb::delete_ &&
		req.method() != http::verb::head)
	{
		send_response(m_server.bad_request("Unknown HTTP-method", req.version()));
		return false;
	}

	// Request path must be absolute and not contain "..".
	if (req.target().empty() ||
	    req.target()[0] != '/' ||
	    req.target().find("..") != boost::beast::string_view::npos)
	{
		send_response(m_server.bad_request("Illegal request-target", req.version()));
		return false;
	}

	return true;

}

template <class Response>
void Session::send_response(Response&& response)
{
	// The lifetime of the message has to extend
	// for the duration of the async operation so
	// we use a shared_ptr to manage it.
	auto sp = std::make_shared<std::remove_reference_t<decltype(response)>>(std::forward<decltype(response)>(response));
	sp->set(http::field::server, BOOST_BEAST_VERSION_STRING);
	sp->keep_alive(m_keep_alive);
	sp->prepare_payload();

	async_write(m_stream, *sp, boost::asio::bind_executor(
		m_strand,
		[self=shared_from_this(), sp](auto&& ec, auto bytes)
		{ self->on_write(ec, bytes, sp->need_eof()); }
	));
}


void Session::handle_read_error(std::string_view where, boost::system::error_code ec)
{
	// This means they closed the connection
	if (ec == boost::beast::http::error::end_of_stream)
		return do_close();

	else if (ec)
	{
		Log(LOG_WARNING, "read error @ %3%: %1% (%2%)", ec, ec.message(), where);
		return send_response(m_server.bad_request(ec.message(), 11));
	}
}

void Session::on_write(
	boost::system::error_code ec,
	std::size_t,
	bool close)
{
//	if (ec)
//		Log(LOG_WARNING, "write error: %1%", ec);

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
	m_stream.async_shutdown(
		boost::asio::bind_executor(
			m_strand,
			[self=shared_from_this()](auto ec){self->on_shutdown(ec);}
		)
	);
}

void Session::on_shutdown(boost::system::error_code)
{
//	if (ec)
//		Log(LOG_WARNING, "shutdown error: %1% (%2%)", ec, ec.message());

	// At this point the connection is closed gracefully
}

} // end of namespace
