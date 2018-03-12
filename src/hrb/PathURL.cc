/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 3/4/18.
//

#include "PathURL.hh"

#include "util/Escape.hh"

#include <sstream>
#include <cassert>

namespace hrb {

PathURL::PathURL(boost::string_view boost_target)
{
	// order is important here
	// All URL path must start with slash
	if (boost_target.empty() || boost_target.front() != '/' || boost_target.size() == 1)
		return;

	// skip the first slash
	std::string_view target{boost_target.data(), boost_target.size()};
	target.remove_prefix(1);

	m_action = target.substr(0, target.find_first_of('/', 0));
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

	m_coll = target;
	while (!m_coll.empty() && m_coll.front() == '/')
		m_coll.remove_prefix(1);
	while (!m_coll.empty() && m_coll.back() == '/')
		m_coll.remove_suffix(1);
}

PathURL::PathURL(std::string_view action, std::string_view user, std::string_view coll, std::string_view name) :
	m_action{action}, m_user{user}, m_coll{coll}, m_filename{name}
{
}

std::string PathURL::str() const
{
	std::ostringstream oss;
	oss << '/' << m_action;
	if (!m_action.empty())
		oss << '/';
	oss << m_user;
	if (!m_user.empty())
		oss << '/';
	oss << m_coll;
	if (!m_coll.empty())
		oss << '/';
	oss << m_filename;

	return oss.str();
}

} // end of namespace hrb
