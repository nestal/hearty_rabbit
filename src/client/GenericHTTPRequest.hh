/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 6/3/18.
//

#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <cstdlib>
#include <functional>
#include <memory>
#include <string>

namespace hrb {

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

// Report a failure
void fail(boost::system::error_code ec, char const *what);

// Performs an HTTP GET and prints the response
template <typename RequestBody, typename ResponseBody>
class GenericHTTPRequest : public std::enable_shared_from_this<
    GenericHTTPRequest<RequestBody, ResponseBody>
>
{
public:
	// Resolver and stream require an io_context
	explicit GenericHTTPRequest(boost::asio::io_context& ioc, ssl::context& ctx) :
		m_resolver(ioc), m_stream(ioc, ctx)
	{
	}

	auto& response() {return m_res;}
	auto& request() {return m_req;}

	template <typename Comp>
	void on_load(Comp&& comp) {m_comp = std::forward<Comp>(comp);}

	// Start the asynchronous operation
	void run(
		std::string_view host,
		std::string_view port,
		std::string_view target,
		http::verb method,
		int version
	)
	{
		// Set SNI Hostname (many hosts need this to handshake successfully)
		if (!SSL_set_tlsext_host_name(m_stream.native_handle(), std::string{host}.c_str()))
		{
			boost::system::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
			return fail(ec, "SSL_set_tlsext_host_name");
		}

		// Set up an HTTP GET request message
		m_req.version(version);
		m_req.method(method);
		m_req.target(std::string{target});
		m_req.set(http::field::host, host);
		m_req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

		// Look up the domain name
		m_resolver.async_resolve(
			host,
			port,
			[self=this->shared_from_this()](auto ec, auto&& results){self->on_resolve(ec, std::move(results));}
		);
	}

private:
	void on_resolve(
		boost::system::error_code ec,
		tcp::resolver::results_type results
	)
	{
		if (ec)
			return fail(ec, "resolve");

		// Make the connection on the IP address we get from a lookup
		boost::asio::async_connect(
			m_stream.next_layer(),
			results.begin(),
			results.end(),
			[self=this->shared_from_this()](auto ec, auto&&){self->on_connect(ec);}
		);
	}

	void on_connect(boost::system::error_code ec)
	{
		if (ec)
			return fail(ec, "connect");

		m_stream.set_verify_mode(boost::asio::ssl::verify_none);

		// Perform the SSL handshake
		m_stream.async_handshake(
			ssl::stream_base::client,
			[self=this->shared_from_this()](auto ec)
			{
				self->on_handshake(ec);
			}
		);
	}

	void on_handshake(boost::system::error_code ec)
	{
		if (ec)
			return fail(ec, "handshake");

		// Send the HTTP request to the remote host
		http::async_write(
			m_stream, m_req,
			[self=this->shared_from_this()](auto ec, auto bytes)
			{
				self->on_write(ec, bytes);
			}
		);
	}

	void on_write(
		boost::system::error_code ec,
		std::size_t bytes_transferred
	)
	{
		boost::ignore_unused(bytes_transferred);

		if (ec)
			return fail(ec, "write");

		// Receive the HTTP response
		http::async_read(
			m_stream, m_buffer, m_res,
			[self=this->shared_from_this()](auto ec, auto bytes)
			{
				self->on_read(ec, bytes);
			}
		);
	}

	void on_read(
		boost::system::error_code ec,
		std::size_t bytes_transferred
	)
	{
		boost::ignore_unused(bytes_transferred);

		if (ec)
			return fail(ec, "read");

		m_comp(ec, *this);

		// Gracefully close the stream
		m_stream.async_shutdown(
			[self=this->shared_from_this()](auto ec)
			{
				self->on_shutdown(ec);
			}
		);
	}

	void on_shutdown(boost::system::error_code ec)
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

private:
	tcp::resolver m_resolver;
	ssl::stream<tcp::socket> m_stream;
	boost::beast::flat_buffer m_buffer; // (Must persist between reads)
	http::request<RequestBody> m_req;
	http::response<ResponseBody> m_res;

	std::function<void(std::error_code, GenericHTTPRequest&)> m_comp;
};

} // end of namespace hrb
