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

#include "util/Escape.hh"

namespace hrb {

PathURL::PathURL(boost::string_view boost_target)
{
	if (boost_target.empty())
		return;

	std::string_view target{boost_target.data(), boost_target.size()};
	m_action = target.substr(0, target.find_first_of('/', 1));
	target.remove_prefix(m_action.size());

	if (target.empty())
		return;

	m_user   = target.substr(0, target.find_first_of('/', 1));
	target.remove_prefix(m_user.size());
	m_user.remove_prefix(1);

	if (target.empty())
		return;

	auto file_start = target.find_last_of('/');
	if (file_start != target.npos) file_start++;
	m_filename = target.substr(file_start, target.size());
	target.remove_suffix(m_filename.size());

	if (target.empty())
		return;

	m_path = target;
}

} // end of namespace hrb
