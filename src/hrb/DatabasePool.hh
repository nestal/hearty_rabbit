/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 2/2/18.
//

#pragma once

#include "net/Redis.hh"

#include <boost/asio/ip/tcp.hpp>
#include <vector>
#include <mutex>

namespace hrb {
/// Test
class DatabasePool
{
public:
	DatabasePool(const boost::asio::ip::tcp::endpoint& remote);

	std::shared_ptr<redis::Connection> alloc(boost::asio::io_context& ioc);
	void release(std::shared_ptr<redis::Connection>&& conn);

	void release_all();

private:
	boost::asio::ip::tcp::endpoint m_remote;

	std::vector<std::shared_ptr<redis::Connection>> m_pool;
	std::mutex  m_mx;
};

} // end of namespace hrb
