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

#include "hrb/Request.hh"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <boost/asio/ip/tcp.hpp>

#include <boost/asio/strand.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/filesystem/path.hpp>

#include <optional>

namespace hrb {

class Server;

// Handles an HTTP server connection
class Session : public std::enable_shared_from_this<Session>
{
public:
	// Take ownership of the socket
	explicit Session(
		boost::asio::ip::tcp::socket socket,
		Server& server,
		boost::asio::ssl::context *ssl_ctx
	);

	// Start the asynchronous operation
	void run();
	void on_handshake(boost::system::error_code ec);
	void do_read();
	void on_read(boost::system::error_code ec, std::size_t bytes_transferred);
	void on_write(boost::system::error_code ec, std::size_t bytes_transferred, bool close);
	void do_close();
	void on_shutdown(boost::system::error_code ec);

private:
	boost::asio::ip::tcp::socket m_socket;
	std::optional<boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>> m_stream;
	boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
	boost::beast::flat_buffer m_buffer;
	Server& m_server;
	Request m_req;
};

} // end of namespace
