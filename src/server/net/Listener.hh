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

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>

#include <optional>
#include <memory>
#include <functional>

namespace hrb {

class Configuration;
class SessionHandler;

// Accepts incoming connections and launches the sessions
class Listener : public std::enable_shared_from_this<Listener>
{
public:
	Listener(
		boost::asio::io_context &ioc,
		std::function<SessionHandler()> factory,
		boost::asio::ssl::context *ssl_ctx,
		const Configuration& m_cfg
	);

	void run();

private:
	void do_accept();
	void on_accept(boost::system::error_code ec);

private:
	boost::asio::ip::tcp::acceptor  m_acceptor;
	boost::asio::ip::tcp::socket    m_socket;
	boost::asio::ssl::context       *m_ssl_ctx{};

	std::function<SessionHandler()> m_session_factory;

	// configurations
	const Configuration&    m_cfg;

	// stats
	std::size_t m_session_count{};
};

} // end of hrb namespace

