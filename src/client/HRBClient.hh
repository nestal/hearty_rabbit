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

#include "GenericHTTPRequest.hh"
#include "common/Cookie.hh"

namespace hrb {

class HRBClient
{
public:
	HRBClient(boost::asio::io_context& ioc, ssl::context& ctx, std::string_view host, std::string_view port);

	template <typename Complete>
	void login(std::string_view user, std::string_view password, Complete&& comp);

private:
	// connection to the server
	boost::asio::io_context&    m_ioc;
	ssl::context&               m_ssl;

	// configurations
	std::string m_host;
	std::string m_port;

	// session cookie
	Cookie m_cookie;
};

}