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

#include "util/Configuration.hh"
#include "util/Log.hh"

#include <boost/asio/strand.hpp>

namespace hrb {

Listener::Listener(
	boost::asio::io_context &ioc,
	std::function<SessionHandler()> session_factory,
	boost::asio::ssl::context *ssl_ctx,
	const Configuration& cfg
) :
	m_ioc{ioc},
	m_acceptor{boost::asio::make_strand(ioc)},
	m_session_factory{session_factory},
	m_ssl_ctx{ssl_ctx},
	m_cfg{cfg}
{
	boost::system::error_code ec;
	auto endpoint = (ssl_ctx ? cfg.listen_https() : cfg.listen_http());

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
		boost::asio::make_strand(m_ioc),
		[self = shared_from_this()](auto ec, boost::asio::ip::tcp::socket socket)
		{
			self->on_accept(ec, std::move(socket));
		}
	);
}

void Listener::on_accept(boost::system::error_code ec, boost::asio::ip::tcp::socket socket)
{
	if (ec)
	{
		Log(LOG_WARNING, "accept error: %1%", ec);
	}
	else if (m_ssl_ctx)
	{
		// Create the session and run it
		std::make_shared<Session>(
			m_session_factory, std::move(socket),
			*m_ssl_ctx, m_session_count,
			m_cfg.session_length(), m_cfg.upload_limit()
		)->run();
		m_session_count++;
	}
	else
	{
		std::make_shared<InsecureSession>(std::move(socket), m_cfg.https_root())->run();
	}

	// Accept another connection
	do_accept();
}

} // end of namespace
