/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 3/4/18.
//

#include "URLIntent.hh"

#include "util/Escape.hh"

#include <sstream>

namespace hrb {

URLIntent::URLIntent(boost::string_view boost_target)
{
	// order is important here
	// All URL path must start with slash
	if (boost_target.empty() || boost_target.front() != '/')
		return;

	// skip the first slash
	std::string_view target{boost_target.data(), boost_target.size()};
	target.remove_prefix(1);

	auto action_str = target.substr(0, target.find_first_of('/', 0));
	target.remove_prefix(action_str.size());
	m_action = parse_action(action_str);
	if (m_action == Action::none)
		return;

	if (target.empty())
		return;

	if (require_user.at(static_cast<std::size_t>(m_action)))
	{
		m_user = target.substr(0, target.find_first_of('/', 1));
		target.remove_prefix(m_user.size());
		m_user.remove_prefix(1);
	}

	if (target.empty())
		return;

	auto option_start = target.find_last_of('?');
	if (option_start != target.npos)
	{
		option_start++;
		m_option = target.substr(option_start, target.size());
		target.remove_suffix(m_option.size());
	}

	if (target.empty())
		return;

	if (target.back() == '?')
		target.remove_suffix(1);

	auto file_start = target.find_last_of('/');
	if (file_start != target.npos)
	{
		file_start++;
		m_filename = target.substr(file_start, target.size());
		target.remove_suffix(m_filename.size());
	}

	if (target.empty())
		return;

	m_coll = trim(target);
	return;
}

URLIntent::URLIntent(Action act, std::string_view user, std::string_view coll, std::string_view name) :
	m_action{act}, m_user{trim(user)}, m_coll{trim(coll)}, m_filename{trim(name)}
{
}

std::string URLIntent::str() const
{
	std::ostringstream oss;
	oss << '/';
	switch (m_action)
	{
		case Action::login:     oss << "login";     break;
		case Action::logout:    oss << "logout";    break;
		case Action::blob:      oss << "blob/";     break;
		case Action::view:      oss << "view/";     break;
		case Action::coll:      oss << "coll/";     break;
		case Action::upload:    oss << "upload/";   break;
		case Action::lib:       oss << "lib/";      break;
		case Action::listcolls: oss << "listcolls/";    break;

		case Action::home:
		case Action::none:
			break;
	}
	oss << m_user;
	if (!m_user.empty())
		oss << '/';
	oss << m_coll;
	if (!m_coll.empty())
		oss << '/';
	oss << m_filename;
	if (!m_option.empty())
		oss << "?" << m_option;

	return oss.str();
}

std::string_view URLIntent::trim(std::string_view s)
{
	while (!s.empty() && s.front() == '/')
		s.remove_prefix(1);
	while (!s.empty() && s.back() == '/')
		s.remove_suffix(1);
	return s;
}

URLIntent::Action URLIntent::parse_action(std::string_view str)
{
	// if the order of occurrence frequency
	if      (str == "blob")     return Action::blob;
	else if (str == "view")     return Action::view;
	else if (str == "coll")     return Action::coll;
	else if (str == "upload")   return Action::upload;
	else if (str == "login")    return Action::login;
	else if (str == "logout")   return Action::logout;
	else if (str == "lib")      return Action::lib;
	else if (str == "listcolls")    return Action::listcolls;
	else if (str.empty())       return Action::home;
	else                        return Action::none;
}

const std::array<bool, static_cast<int>(URLIntent::Action::none)> URLIntent::require_user =
//   login, logout, blob, view, coll, upload, home,  lib,   listcolls, none
	{false, false,  true, true, true, true,   false, false, true};

const std::array<bool, static_cast<int>(URLIntent::Action::none)> URLIntent::require_filename =
//   login, logout, blob, view,  coll,  upload, home,  lib,  listcolls, none
	{false, false,  true, false, false, true,   false, true, false};

bool URLIntent::valid() const
{
	if (m_action != Action::none)
	{
		// collection may be empty in any case
		return
			!m_user.empty()     == require_user.at(static_cast<std::size_t>(m_action)) &&
			!m_filename.empty() == require_filename.at(static_cast<std::size_t>(m_action));
	}
	else
	{
		return false;
	}
}

} // end of namespace hrb
