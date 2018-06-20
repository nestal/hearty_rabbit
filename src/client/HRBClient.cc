/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 6/3/18.
//

#include "HRBClient.hh"

#include "GenericHTTPRequest.hh"
#include "common/URLIntent.hh"

namespace hrb {

HRBClient::HRBClient(
	boost::asio::io_context& ioc,
	boost::asio::ssl::context& ctx,
	std::string_view host,
	std::string_view port
) :
	m_ioc{ioc}, m_ssl{ctx}, m_host{host}, m_port{port}
{
}

std::shared_ptr<HRBClient::CommonRequest> HRBClient::request(const URLIntent& intent, boost::beast::http::verb method)
{
	auto req = std::make_shared<CommonRequest>(m_ioc, m_ssl);
	req->init(m_host, m_port, intent.str(), http::verb::get);
	req->request().set(http::field::cookie, m_user.cookie().str());
	return req;
}

} // end of namespace
