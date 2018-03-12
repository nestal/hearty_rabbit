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

InsecureSession::InsecureSession(boost::asio::ip::tcp::socket socket, unsigned short https_port) :
	m_socket{std::move(socket)},
	m_strand{m_socket.get_executor()},
	m_https_port{https_port}
{
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
	std::cout << "on insecure request: \"" << m_request[http::field::host] << "\" " << m_request.target() << std::endl;

	http::response<http::empty_body> res{http::status::moved_permanently, m_request.version()};
	res.set(http::field::location, redirect(
		{m_request[http::field::host].data(), m_request[http::field::host].size()},
		m_https_port)
	);
	res.prepare_payload();

	async_write(m_socket, res, boost::asio::bind_executor(
		m_strand,
		[self=shared_from_this()](auto&& ec, auto)
		{
			self->m_socket.shutdown(tcp::socket::shutdown_send, ec);
		}
	));
}

std::string InsecureSession::redirect(std::string_view host, unsigned short port)
{
	std::ostringstream oss;
	oss << "https://" << host.substr(0, host.find(':'));
	if (port != 443)
		oss << ':' << port;
	return oss.str();
}

} // end of namespace hrb
