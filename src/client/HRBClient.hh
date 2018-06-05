/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 6/3/18.
//

#pragma once

#include "common/Cookie.hh"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl.hpp>

namespace hrb {

class HRBClient
{
public:
	HRBClient(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, std::string_view host, std::string_view port);

	template <typename Complete>
	void login(std::string_view user, std::string_view password, Complete&& comp);

	template <typename Complete>
	void list(std::string_view coll, Complete&& comp);

private:
	// connection to the server
	boost::asio::io_context&    m_ioc;
	boost::asio::ssl::context&  m_ssl;

	// configurations
	std::string m_host;
	std::string m_port;

	// session cookie
	Cookie m_cookie;
	std::string m_user;
};

}