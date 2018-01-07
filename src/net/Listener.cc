/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/1/18.
//

#include "Listener.hh"
#include "Session.hh"

#include "util/Log.hh"

namespace hrb {

Listener::Listener(
	boost::asio::io_context &ioc,
	boost::asio::ip::tcp::endpoint endpoint,
	Server& server,
	boost::asio::ssl::context *ssl_ctx
) :
	m_acceptor{ioc},
	m_socket{ioc},
	m_server{server},
	m_ssl_ctx{ssl_ctx}
{
	boost::system::error_code ec;

	// Open the acceptor
	m_acceptor.open(endpoint.protocol(), ec);
	if (ec)
		throw std::system_error(ec);

	// Bind to the server address
	m_acceptor.bind(endpoint, ec);
	if (ec)
		throw std::system_error(ec);

	// Start listening for connections
	m_acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
	if (ec)
		throw std::system_error(ec);
}

void Listener::run()
{
	if (m_acceptor.is_open())
		do_accept();
}

void Listener::do_accept()
{
	m_acceptor.async_accept(
		m_socket,
		[self = shared_from_this()](auto ec)
		{
			self->on_accept(ec);
		}
	);
}

void Listener::on_accept(boost::system::error_code ec)
{
	if (ec)
	{
		LOG(LOG_WARNING, "accept error: %d (%s)", ec.value(), ec.message());
	}
	else
	{
		// Create the session and run it
		std::make_shared<Session>(std::move(m_socket), m_server, m_ssl_ctx)->run();
	}

	// Accept another connection
	do_accept();
}

} // end of namespace
