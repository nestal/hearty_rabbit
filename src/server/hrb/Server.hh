/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/7/18.
//

#pragma once

#include "WebResources.hh"

#include "net/Redis.hh"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>

#include <system_error>
#include <functional>

namespace hrb {

class Configuration;
class Password;
class SessionHandler;

/// The main application logic of hearty rabbit.
/// This is the class that handles HTTP requests from and produce response to clients. The unit test
/// this class calls handle_https().
class Server
{
public:
	explicit Server(const Configuration& cfg);
	void listen();

	boost::asio::io_context& get_io_context();

	SessionHandler start_session();

	// Administrative commands and configurations
	static void add_user(
		const Configuration& cfg,
		std::string_view username,
		Password&& password,
		std::function<void(std::error_code)> complete
	);

	void drop_privileges() const;

private:
	const Configuration&        m_cfg;
	boost::asio::ssl::context   m_ssl{boost::asio::ssl::context::sslv23};
	boost::asio::io_context     m_ioc;

	redis::Pool     m_db;
	WebResources    m_lib;
};

} // end of namespace
