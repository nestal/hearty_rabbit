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

#include "hrb/SessionHandler.ipp"
#include "net/SplitBuffers.hh"
#include "util/Cookie.hh"
#include "util/Error.hh"
#include "util/Log.hh"

#include <boost/asio/bind_executor.hpp>

namespace hrb {

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

Session::Session(
	std::function<SessionHandler()> factory,
	boost::asio::ip::tcp::socket socket,
	boost::asio::ssl::context&  ssl_ctx,
	std::size_t             nth,
	std::chrono::seconds    login_session,
	std::size_t             upload_limit
) :
	m_socket{std::move(socket)},
	m_stream{m_socket, ssl_ctx},
	m_factory{std::move(factory)},
	m_nth_session{nth},
	m_login_session{login_session},
	m_upload_size_limit{upload_limit}
{
}

// Start the asynchronous operation
void Session::run()
{
	// Perform the SSL handshake
	m_stream.async_handshake(
		boost::asio::ssl::stream_base::server,
		[self = shared_from_this()](auto ec){self->on_handshake(ec);}
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

	m_handler.emplace(m_factory());
	m_parser->body_limit(m_upload_size_limit);

	// Read the header of a request
	async_read_header(
		m_stream, m_buffer, *m_parser,
		[self=shared_from_this()](auto ec, auto bytes) {self->on_read_header(ec, bytes);}
	);
}


void Session::on_read_header(boost::system::error_code ec, std::size_t)
{
	if (ec)
		handle_read_error(__PRETTY_FUNCTION__, {ec.value(), ec.category()});
	else
	{
		// Get the HTTP header from the partially parsed request message from the parser.
		// The body of the request message has not parsed yet.
		auto&& header = m_parser->get();
		m_keep_alive = header.keep_alive();

		if (!validate_request(header))
			return;

		assert(m_handler.has_value());
		m_handler->on_request_header(
			header,
			[self=shared_from_this(), this](SessionHandler::RequestBodyType body_type, std::error_code ec)
			{
				init_request_body(body_type, ec);
				if (ec)
				{
					Log(LOG_WARNING, "cannot initialize request parser: %1% (%2%)", ec.message(), ec);
					return send_response(m_handler->server_error("internal server error", 11));
				}

//				assert(m_handler->auth().invariance());

				// Call async_read() using the chosen parser to read and parse the request body.
				std::visit([this, self](auto&& parser)
				{
					boost::beast::http::async_read(
						m_stream, m_buffer, parser,
						[self](auto ec, auto bytes){ self->on_read(ec, bytes); }
					);
				}, m_body);
			}
		);
	}
}


void Session::on_read(boost::system::error_code ec, std::size_t)
{
	assert(m_handler.has_value());
//	assert(m_handler->auth().invariance());

	// This means they closed the connection
	if (ec)
		return handle_read_error(__PRETTY_FUNCTION__, ec);
	else
	{
		std::visit([this, self=shared_from_this()](auto&& parser)
		{
			m_handler->on_request_body(
				parser.release(), [this, self](auto&& response)
				{
//					assert(m_handler->auth().invariance());

					// server must not set session cookie
					assert(response.count(http::field::set_cookie) == 0);

					// check if auth is renewed. if yes, set it to cookie before sending
					if (m_handler->renewed_auth())
					{
//						response.set(http::field::set_cookie, m_handler->auth().set_cookie(m_login_session).str());
					}

					send_response(std::forward<decltype(response)>(response));
				}
			);
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
	assert(m_handler.has_value());

	boost::system::error_code ec;
	auto endpoint = m_socket.remote_endpoint(ec);
	if (ec)
		Log(LOG_WARNING, "remote_endpoint() error: %1% %2%", ec, ec.message());

	// Make sure we can handle the method
	if (req.method() != http::verb::get  &&
	    req.method() != http::verb::post &&
	    req.method() != http::verb::put &&
		req.method() != http::verb::delete_ &&
		req.method() != http::verb::head)
	{
		send_response(m_handler->bad_request("Unknown HTTP-method", req.version()));
		return false;
	}

	// Request path must be absolute and not contain "..".
	if (req.target().empty() ||
	    req.target()[0] != '/' ||
	    req.target().find("..") != boost::beast::string_view::npos)
	{
		send_response(m_handler->bad_request("Illegal request-target", req.version()));
		return false;
	}
	return true;
}

template <class Response>
void Session::send_response(Response&& response)
{
	static const std::string version =
		(boost::format{"%1% HeartyRabbit/%2%"} % BOOST_BEAST_VERSION_STRING % hrb::constants::version).str();

	// The lifetime of the message has to extend
	// for the duration of the async operation so
	// we use a shared_ptr to manage it.
	auto sp = std::make_shared<std::remove_reference_t<Response>>(std::forward<Response>(response));
	sp->set(http::field::server, version);
	sp->keep_alive(m_keep_alive);
	sp->prepare_payload();

	async_write(
		m_stream, *sp,
		[self=shared_from_this(), sp](auto&& ec, auto bytes)
		{
			self->on_write(ec, bytes, sp->need_eof());
		}
	);
}

void Session::handle_read_error(std::string_view where, boost::system::error_code ec)
{
	assert(m_handler.has_value());
	// This means they closed the connection
	if (ec == boost::beast::http::error::end_of_stream)
		return do_close();

	else if (ec)
	{
		return send_response(m_handler->bad_request(ec.message(), 11));
	}
}

void Session::on_write(
	boost::system::error_code ec,
	std::size_t,
	bool close)
{
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
		[self=shared_from_this()](auto ec){self->on_shutdown(ec);}
	);
}

void Session::on_shutdown(boost::system::error_code)
{
	// At this point the connection is closed gracefully
}

void Session::init_request_body(SessionHandler::RequestBodyType body_type, std::error_code& ec)
{
	switch (body_type)
	{
	case SessionHandler::RequestBodyType::empty:
		m_body.emplace<EmptyRequestParser>(std::move(*m_parser));
		break;

	case SessionHandler::RequestBodyType::string:
		m_body.emplace<StringRequestParser>(std::move(*m_parser));
		break;

	case SessionHandler::RequestBodyType::upload:
		auto& parser = m_body.emplace<UploadRequestParser>(std::move(*m_parser));
		m_handler->prepare_upload(parser.get().body(), ec);
		break;
	}

}

} // end of namespace
