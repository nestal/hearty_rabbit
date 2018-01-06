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
#include <boost/filesystem/path.hpp>

namespace hrb {

// Accepts incoming connections and launches the sessions
class Listener : public std::enable_shared_from_this<Listener>
{
public:
	Listener(
		boost::asio::io_context &ioc,
		boost::asio::ip::tcp::endpoint endpoint,
		boost::asio::ip::tcp::endpoint endpoint_tls,
		const boost::filesystem::path& doc_root,
		boost::asio::ssl::context& ssl_ctx
	);

	void run();

	void do_accept();
	void on_accept(boost::system::error_code ec);
	void do_accept_tls();
	void on_accept_tls(boost::system::error_code ec);

private:
	boost::asio::ip::tcp::acceptor m_acceptor, m_acceptor_tls;
	boost::asio::ip::tcp::socket   m_socket, m_socket_tls;
	boost::filesystem::path        m_doc_root;
	boost::asio::ssl::context&     m_ssl_ctx;
};

} // end of hrb namespace

