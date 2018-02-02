/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 2/2/18.
//

#include "DatabasePool.hh"

namespace hrb {

DatabasePool::DatabasePool(boost::asio::io_context& ioc, std::string_view host, unsigned short port) :
	m_ioc{ioc},
	m_host{host},
	m_port{port}
{
}

std::shared_ptr<redis::Connection> DatabasePool::alloc()
{
	std::unique_lock<std::mutex> lock{m_mx};

	if (m_pool.empty())
		return std::make_shared<redis::Connection>(m_ioc, m_host, m_port);

	auto conn{std::move(m_pool.back())};
	m_pool.pop_back();
	return conn;
}

void DatabasePool::release(std::shared_ptr<redis::Connection>&& conn)
{
	std::unique_lock<std::mutex> lock{m_mx};
	m_pool.push_back(std::move(conn));
}

void DatabasePool::release_all()
{
	std::unique_lock<std::mutex> lock{m_mx};
	for (auto&& db : m_pool)
		db->disconnect();

	m_ioc.post([this]{m_pool.clear();});
}

} // end of namespace hrb
