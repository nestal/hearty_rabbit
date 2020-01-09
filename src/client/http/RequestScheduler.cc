/*
	Copyright © 2020 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 9/1/2020.
//

#include "RequestScheduler.hh"
#include "GenericHTTPRequest.hh"

namespace hrb {

void RequestScheduler::add(std::shared_ptr<BaseRequest>&& req)
{
	m_pending.push_back(std::move(req));
	try_start();
}

void RequestScheduler::try_start()
{
	while (m_outstanding.size() < m_limit && !m_pending.empty())
	{
		auto req = std::move(m_pending.front());
		m_pending.pop_front();

		req->run();
		m_outstanding.insert(std::move(req));
	}
}

void RequestScheduler::finish(const std::shared_ptr<BaseRequest>& req)
{
	m_outstanding.erase(req);
	try_start();
}

} // end of namespace hrb
