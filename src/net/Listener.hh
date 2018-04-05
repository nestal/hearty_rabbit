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

class Session;

// Accepts incoming connections and launches the sessions
class Listener : public std::enable_shared_from_this<Listener>
{
public:
	using SessionFactory = std::function<std::shared_ptr<Session>(
		boost::asio::ip::tcp::socket&&,
		boost::asio::ssl::context&,
		std::size_t
	)>;

	Listener(
		boost::asio::io_context &ioc,
		boost::asio::ip::tcp::endpoint endpoint,
		SessionFactory session_factory,
		boost::asio::ssl::context *ssl_ctx,
		std::string&& https_root
	);

	void run();

private:
	void do_accept();
	void on_accept(boost::system::error_code ec);

private:
	boost::asio::ip::tcp::acceptor  m_acceptor;
	boost::asio::ip::tcp::socket    m_socket;
	SessionFactory                  m_session_factory;
	boost::asio::ssl::context       *m_ssl_ctx{};
	std::string                     m_https_root;

	// stats
	std::size_t m_session_count{};
};

} // end of hrb namespace

