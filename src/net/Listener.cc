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
#include "InsecureSession.hh"

#include "util/Log.hh"

namespace hrb {

Listener::Listener(
	boost::asio::io_context &ioc,
	boost::asio::ip::tcp::endpoint endpoint,
	SessionFactory session_factory,
	boost::asio::ssl::context *ssl_ctx,
	const std::string& https_root
) :
	m_acceptor{ioc},
	m_socket{ioc},
	m_session_factory{session_factory},
	m_ssl_ctx{ssl_ctx},
	m_https_root{https_root}
{
	boost::system::error_code ec;

	// Open the acceptor
	m_acceptor.open(endpoint.protocol(), ec);
	if (ec)
		throw std::system_error(ec);

	m_acceptor.set_option(boost::asio::socket_base::reuse_address{true});

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
		Log(LOG_WARNING, "accept error: %1%", ec);
	}
	else if (m_ssl_ctx)
	{
		// Create the session and run it
		m_session_factory(std::move(m_socket), *m_ssl_ctx, m_session_count)->run();
		m_session_count++;
	}
	else
	{
		std::make_shared<InsecureSession>(std::move(m_socket), m_https_root)->run();
	}

	// Accept another connection
	do_accept();
}

} // end of namespace
