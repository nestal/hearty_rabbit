/*
	Copyright © 2020 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 8/1/2020.
//

#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <utility>

namespace hrb {

template <typename Callback>
class AggregatedCallBack
{
public:
	AggregatedCallBack(Callback&& callback, std::size_t expected_count) :
		m_callback{std::move(callback)},
		m_expected_count{expected_count},
		m_count{0}
	{
	}

	template <typename... Args>
	void operator()(Args&& ... args)
	{
		if (++m_count == m_expected_count)
			std::invoke(m_callback, std::forward<Args>(args)...);
	}

private:
	Callback    m_callback;
	std::size_t m_expected_count;
	std::size_t m_count;
};

template <typename Callback>
auto make_shared_callback(Callback&& callback, std::size_t expected_count)
{
	return std::make_shared<AggregatedCallBack<std::decay_t<Callback>>>(
		std::forward<Callback>(callback), expected_count
	);
}

} // end of namespace hrb
