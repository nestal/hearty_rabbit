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

#include <systemd/sd-journal.h>
#include <iostream>

namespace hrb {

Listener::Listener(
	boost::asio::io_context &ioc,
	boost::asio::ip::tcp::endpoint endpoint,
	boost::asio::ip::tcp::endpoint endpoint_tls,
	const boost::filesystem::path& doc_root,
	boost::asio::ssl::context& ssl_ctx
) :
	m_acceptor{ioc},
	m_acceptor_tls{ioc},
	m_socket{ioc},
	m_socket_tls{ioc},
	m_doc_root{doc_root},
	m_ssl_ctx{ssl_ctx}
{
	boost::system::error_code ec;

	// Open the acceptor
	m_acceptor.open(endpoint.protocol(), ec);
	if (ec)
		throw std::system_error(ec);
	m_acceptor_tls.open(endpoint_tls.protocol(), ec);
	if (ec)
		throw std::system_error(ec);

	// Bind to the server address
	m_acceptor.bind(endpoint, ec);
	if (ec)
		throw std::system_error(ec);
	m_acceptor_tls.bind(endpoint_tls, ec);
	if (ec)
		throw std::system_error(ec);

	// Start listening for connections
	m_acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
	if (ec)
		throw std::system_error(ec);
	m_acceptor_tls.listen(boost::asio::socket_base::max_listen_connections, ec);
	if (ec)
		throw std::system_error(ec);

	// drop privileges if run as root
	if (::getuid() == 0)
	{
		if (setuid(65535) != 0)
			throw std::system_error(errno, std::system_category());
	}

	if (::getuid() == 0)
		throw std::runtime_error("cannot run as root");
}

void Listener::run()
{
	if (m_acceptor.is_open())
		do_accept(EnableTLS::no_tls);
	if (m_acceptor_tls.is_open())
		do_accept(EnableTLS::use_tls);
}

void Listener::do_accept(EnableTLS tls)
{
	auto&& acceptor = (tls ? m_acceptor_tls : m_acceptor);
	acceptor.async_accept(
		(tls ? m_socket_tls : m_socket),
		[self = shared_from_this(), tls](auto ec)
		{
			self->on_accept(ec, tls);
		}
	);
}

void Listener::on_accept(boost::system::error_code ec, EnableTLS tls)
{
	if (ec)
	{
		sd_journal_print(LOG_WARNING, "accept error: %d (%s)", ec.value(), ec.message());
	}
	else
	{
		// Create the session and run it
		std::make_shared<Session>(std::move(tls ? m_socket_tls : m_socket), m_doc_root, m_ssl_ctx)->run();
	}

	// Accept another connection
	do_accept(tls);
}

} // end of namespace
