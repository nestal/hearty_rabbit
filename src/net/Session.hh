/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/1/18.
//

#pragma once

#include "Request.hh"
#include "crypto/Authentication.hh"
#include "hrb/UploadFile.hh"
#include "hrb/URLIntent.hh"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <boost/asio/ip/tcp.hpp>

#include <boost/asio/strand.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <optional>

namespace hrb {

class Server;
class Authentication;

// Handles an HTTP server connection
class Session : public std::enable_shared_from_this<Session>
{
public:
	// Take ownership of the socket
	explicit Session(
		Server& server,
		boost::asio::ip::tcp::socket socket,
		boost::asio::ssl::context& ssl_ctx,
		std::size_t nth
	);

	// Start the asynchronous operation
	void run();
	void on_handshake(boost::system::error_code ec);
	void do_read();
	void on_read_header(boost::system::error_code ec, std::size_t bytes_transferred);
	void on_read(boost::system::error_code ec, std::size_t bytes_transferred, bool auth_renewed);
	void on_write(boost::system::error_code ec, std::size_t bytes_transferred, bool close);
	void do_close();
	void on_shutdown(boost::system::error_code ec);

private:
	// This function produces an HTTP response for the given
	// request. The type of the response object depends on the
	// contents of the request, so the interface requires the
	// caller to pass a generic lambda for receiving the response.
	template<class Request>
	bool validate_request(const Request& req);

	template <class Response>
	void send_response(Response&& response);

	void handle_read_error(std::string_view where, boost::system::error_code ec);

private:
	tcp::socket		                                            m_socket;
	boost::asio::ssl::stream<tcp::socket&> 		                m_stream;
	boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
	boost::beast::flat_buffer                                   m_buffer;

	Authentication	m_auth;

	bool m_keep_alive{false};

	// The parsed message are stored inside the parsers.
	// Use parser::get() or release() to get the message.
	std::optional<EmptyRequestParser> m_parser;
	std::variant<StringRequestParser, UploadRequestParser, EmptyRequestParser> m_body;

	Server&     m_server;

	// stats
	std::size_t m_nth_session;
	std::size_t m_nth_transaction{};
};

} // end of namespace
