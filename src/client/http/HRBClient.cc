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
#include "hrb/URLIntent.hh"

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


void HRBClient::add_request(std::shared_ptr<BaseRequest>&& req)
{
	m_pending.push_back(std::move(req));
	try_start_requests();
}

void HRBClient::try_start_requests()
	{
	while (m_outstanding.size() < m_outstanding_limit && !m_pending.empty())
	{
		auto req = std::move(m_pending.front());
		m_pending.pop_front();

		req->run();
		m_outstanding.insert(std::move(req));
	}
}

void HRBClient::finish_request(const std::shared_ptr<BaseRequest>& req)
{
	m_outstanding.erase(req);
	try_start_requests();
}

} // end of namespace
