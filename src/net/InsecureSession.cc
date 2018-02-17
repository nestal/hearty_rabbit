/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/17/18.
//

#include "InsecureSession.hh"

#include <boost/asio/bind_executor.hpp>
#include <iostream>

namespace hrb {

InsecureSession::InsecureSession(boost::asio::ip::tcp::socket socket, std::string_view redirect) :
	m_socket{std::move(socket)},
	m_strand{m_socket.get_executor()},
	m_response{http::status::moved_permanently, 11}
{
	m_response.set(http::field::location, redirect);
	m_response.prepare_payload();
}

void InsecureSession::run()
{
	do_read();
}

void InsecureSession::do_read()
{
	async_read(m_socket, m_buffer, m_request, boost::asio::bind_executor(
		m_strand,
		[self=shared_from_this()](auto ec, auto bytes) {self->on_read(ec, bytes);}
	));
}

void InsecureSession::on_read(boost::beast::error_code ec, std::size_t)
{
	std::cout << "on request: " << m_request.target() << std::endl;
	async_write(m_socket, m_response, boost::asio::bind_executor(
		m_strand,
		[self=shared_from_this()](auto&& ec, auto)
		{
			self->m_socket.shutdown(tcp::socket::shutdown_send, ec);
		}
	));
}

} // end of namespace hrb
