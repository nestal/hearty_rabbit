/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/17/18.
//

#include "Request.hh"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <boost/asio/ip/tcp.hpp>

#include <boost/asio/strand.hpp>

#include <memory>

#pragma once

namespace hrb {

class InsecureSession : public std::enable_shared_from_this<InsecureSession>
{
public:
	InsecureSession(
		boost::asio::ip::tcp::socket socket,
		unsigned short https_port
	);

	void run();

	static std::string redirect(std::string_view host, unsigned short https_port);

private:
	void do_read();
	void on_read(boost::beast::error_code ec, std::size_t);

private:
	tcp::socket		                                            m_socket;
	boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
	boost::beast::flat_buffer                                   m_buffer;

	EmptyRequest	m_request;
	unsigned short	m_https_port;
};

} // end of namespace hrb
