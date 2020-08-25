/*
	Copyright © 2020 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 25/8/2020.
//

#include "Postgres.hh"

#include <cassert>

namespace hrb::postgres {

Session::Session(boost::asio::io_context& ioc, const std::string& connection_string) :
	m_socket{ioc},
	m_conn{::PQconnectdb(connection_string.c_str())}
{
	if (!m_conn)
		throw -1;

	m_socket.assign(boost::asio::ip::tcp::v4(), ::PQsocket(m_conn));
}

Session::~Session()
{
	assert(m_conn);
	::PQfinish(m_conn);
}

Result::Result(::PGresult *result) : m_result{result}
{
}

Result::~Result()
{
	if (m_result)
		::PQclear(m_result);
}

} // end of namespace hrb::postgres
