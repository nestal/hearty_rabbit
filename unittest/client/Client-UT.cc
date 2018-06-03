/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 6/3/18.
//

#include <catch.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

//------------------------------------------------------------------------------

// Report a failure
void
fail(boost::system::error_code ec, char const *what)
{
	std::cerr << what << ": " << ec.message() << "\n";
}

// Performs an HTTP GET and prints the response
class session : public std::enable_shared_from_this<session>
{
	tcp::resolver resolver_;
	ssl::stream<tcp::socket> stream_;
	boost::beast::flat_buffer buffer_; // (Must persist between reads)
	http::request<http::empty_body> req_;
	http::response<http::string_body> res_;

public:
	// Resolver and stream require an io_context
	explicit
	session(boost::asio::io_context& ioc, ssl::context& ctx)
		:
		resolver_(ioc), stream_(ioc, ctx)
	{
	}

	// Start the asynchronous operation
	void
	run(
		std::string_view host,
		std::string_view port,
		std::string_view target,
		int version
	)
	{
		// Set SNI Hostname (many hosts need this to handshake successfully)
		if (!SSL_set_tlsext_host_name(stream_.native_handle(), std::string{host}.c_str()))
		{
			boost::system::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
			std::cerr << ec.message() << "\n";
			return;
		}

		// Set up an HTTP GET request message
		req_.version(version);
		req_.method(http::verb::get);
		req_.target(std::string{target});
		req_.set(http::field::host, host);
		req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

		// Look up the domain name
		resolver_.async_resolve(
			host,
			port,
			std::bind(
				&session::on_resolve,
				shared_from_this(),
				std::placeholders::_1,
				std::placeholders::_2
			));
	}

	void
	on_resolve(
		boost::system::error_code ec,
		tcp::resolver::results_type results
	)
	{
		if (ec)
			return fail(ec, "resolve");

		// Make the connection on the IP address we get from a lookup
		boost::asio::async_connect(
			stream_.next_layer(),
			results.begin(),
			results.end(),
			std::bind(
				&session::on_connect,
				shared_from_this(),
				std::placeholders::_1
			));
	}

	void
	on_connect(boost::system::error_code ec)
	{
		if (ec)
			return fail(ec, "connect");

		stream_.set_verify_mode(boost::asio::ssl::verify_none);

		// Perform the SSL handshake
		stream_.async_handshake(
			ssl::stream_base::client,
			std::bind(
				&session::on_handshake,
				shared_from_this(),
				std::placeholders::_1
			));
	}

	void
	on_handshake(boost::system::error_code ec)
	{
		if (ec)
			return fail(ec, "handshake");

		// Send the HTTP request to the remote host
		http::async_write(
			stream_, req_,
			std::bind(
				&session::on_write,
				shared_from_this(),
				std::placeholders::_1,
				std::placeholders::_2
			));
	}

	void
	on_write(
		boost::system::error_code ec,
		std::size_t bytes_transferred
	)
	{
		boost::ignore_unused(bytes_transferred);

		if (ec)
			return fail(ec, "write");

		// Receive the HTTP response
		http::async_read(
			stream_, buffer_, res_,
			std::bind(
				&session::on_read,
				shared_from_this(),
				std::placeholders::_1,
				std::placeholders::_2
			));
	}

	void
	on_read(
		boost::system::error_code ec,
		std::size_t bytes_transferred
	)
	{
		boost::ignore_unused(bytes_transferred);

		if (ec)
			return fail(ec, "read");

		// Write the message to standard out
		std::cout << res_ << std::endl;

		// Gracefully close the stream
		stream_.async_shutdown(
			std::bind(
				&session::on_shutdown,
				shared_from_this(),
				std::placeholders::_1
			));
	}

	void
	on_shutdown(boost::system::error_code ec)
	{
		if (ec == boost::asio::error::eof)
		{
			// Rationale:
			// http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
			ec.assign(0, ec.category());
		}
		if (ec)
			return fail(ec, "shutdown");

		// If we get here then the connection is closed gracefully
	}
};

//------------------------------------------------------------------------------

TEST_CASE("simple client test", "[normal]")
{
	std::string host = "localhost";
	std::string port = "4433";
	std::string target = "/api/nestal";
	int version = 11;

	// The io_context is required for all I/O
	boost::asio::io_context ioc;

	// The SSL context is required, and holds certificates
	ssl::context ctx{ssl::context::sslv23_client};

	// Launch the asynchronous operation
	std::make_shared<session>(ioc, ctx)->run(host, port, target, version);

	// Run the I/O service. The call will return when
	// the get operation is complete.
	ioc.run();
}
