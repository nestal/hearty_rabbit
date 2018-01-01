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

#include "Session.hh"

#include <boost/asio/ip/tcp.hpp>

namespace hrb {

// Accepts incoming connections and launches the sessions
class Listener : public std::enable_shared_from_this<Listener>
{
private:
	boost::asio::ip::tcp::acceptor m_acceptor;
	boost::asio::ip::tcp::socket m_socket;
	std::string m_doc_root;

public:
	Listener(
		boost::asio::io_context &ioc,
		boost::asio::ip::tcp::endpoint endpoint,
		const std::string &doc_root)
		: m_acceptor(ioc), m_socket(ioc), m_doc_root(doc_root)
	{
		boost::system::error_code ec;

		// Open the acceptor
		m_acceptor.open(endpoint.protocol(), ec);
		if (ec)
		{
//            fail(ec, "open");
			return;
		}

		// Bind to the server address
		m_acceptor.bind(endpoint, ec);
		if (ec)
		{
//            fail(ec, "bind");
			return;
		}

		// Start listening for connections
		m_acceptor.listen(
			boost::asio::socket_base::max_listen_connections, ec);
		if (ec)
		{
//            fail(ec, "listen");
			return;
		}
	}

	// Start accepting incoming connections
	void run()
	{
		if (m_acceptor.is_open())
			do_accept();
	}

	void do_accept()
	{
		m_acceptor.async_accept(
			m_socket,
			[self = shared_from_this()](boost::system::error_code ec)
			{ self->on_accept(ec); }
		);
	}

	void on_accept(boost::system::error_code ec)
	{
		if (ec)
		{
//			fail(ec, "accept");
			throw -1;
		}
		else
		{
			// Create the session and run it
			std::make_shared<Session>(std::move(m_socket), m_doc_root)->run();
		}

		// Accept another connection
		do_accept();
	}
};

} // end of hrb namespace

