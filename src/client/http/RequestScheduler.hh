/*
	Copyright © 2020 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 9/1/2020.
//

#pragma once

#include <unordered_set>
#include <deque>
#include <memory>

namespace hrb {

class BaseRequest;

class RequestScheduler
{
public:
	explicit RequestScheduler(std::size_t limit = 5) : m_limit{limit} {}

	void add(std::shared_ptr<BaseRequest>&& req);
	void finish(const std::shared_ptr<BaseRequest>& req);
	void try_start();

private:
	std::size_t m_limit;

	std::unordered_set<std::shared_ptr<BaseRequest>> m_outstanding;
	std::deque<std::shared_ptr<BaseRequest>> m_pending;
};

} // end of namespace hrb
