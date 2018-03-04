/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 3/4/18.
//

#include <iostream>
#include "PathURL.hh"

namespace hrb {

PathURL::PathURL(boost::string_view boost_target)
{
	if (boost_target.empty())
		return;

	std::string_view target{boost_target.data(), boost_target.size()};
	auto action_end = target.find_first_of('/', 1);
	auto file_start = action_end == target.npos ? target.npos : target.find_last_of('/', target.size());

	if (file_start != target.npos)
		file_start++;

	std::cout << "file_start = " << file_start << " " << " action_end = " << action_end << std::endl;

	m_action   = target.substr(0, action_end);
	m_path     = action_end == target.npos ? std::string_view{} : target.substr(action_end, file_start-action_end);
	m_filename = file_start == target.npos ? std::string_view{} : target.substr(file_start);
}

} // end of namespace hrb
