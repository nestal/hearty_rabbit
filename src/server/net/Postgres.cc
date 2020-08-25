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

namespace hrb::postgres {

Session::Session(boost::asio::io_context& ioc) : m_socket{ioc}
{
}

Session::~Session()
{
	if (m_conn)
		::PQfinish(m_conn);
}

void Session::open(const std::string& connection_string)
{
	assert(m_conn == nullptr);
	m_conn = ::PQconnectdb(connection_string.c_str());
	if (m_conn)
		m_socket.assign(boost::asio::ip::tcp::v4(), ::PQsocket(m_conn));
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
