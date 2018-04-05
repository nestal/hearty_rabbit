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
#include "ObjectID.hh"

#include <sstream>
#include <cassert>

namespace hrb {

const std::array<
    URLIntent::Parameters,
    static_cast<int>(URLIntent::Action::none)
> URLIntent::intent_defintions{
	// login, logout
	Parameters{}, Parameters{},

	// view
	Parameters{
		URLIntent::Parameter::user,
		URLIntent::Parameter::collection,
		URLIntent::Parameter::blob,
		URLIntent::Parameter::option
	},

	// list
	Parameters{URLIntent::Parameter::user, URLIntent::Parameter::collection},

	// upload
	Parameters{URLIntent::Parameter::user, URLIntent::Parameter::collection, URLIntent::Parameter::filename},

	// home
	Parameters{},

	// lib
	Parameters{URLIntent::Parameter::filename},

	// listcolls
	Parameters{URLIntent::Parameter::user},

	// query
	Parameters{URLIntent::Parameter::command, URLIntent::Parameter::filename}
};

URLIntent::URLIntent(boost::string_view boost_target)
{
	// order is important here
	// All URL path must start with slash
	if (boost_target.empty() || boost_target.front() != '/')
		return;

	// skip the first slash
	std::string_view target{boost_target.data(), boost_target.size()};
	target.remove_prefix(1);

	auto [action_str, sep] = split_left(target, "/?#");
	m_action = parse_action(action_str);
	if (m_action == Action::none)
		return;

	if (target.empty())
		return;

	if (!forbid_user.at(static_cast<std::size_t>(m_action)))
	{
		auto [user, sep] = split_left(target, "/?#");
		m_user = url_decode(user);
	}

	if (target.empty())
		return;

	auto [option, sep2] = split_right(target, "?", true);
	if (sep2 == '?')
		m_option = option;

	if (target.empty())
		return;

	// The action allows a filename at the end of the URL -> extract the filename
	// from the end of string to the last slash.
	if (!forbid_filename.at(static_cast<std::size_t>(m_action)))
	{
		// After split_right() remove the matched string from "target", we can't undo it.
		// Therefore we make a copy of target and match that instead.
		auto target_copy = target;
		auto [filename, sep3] = split_right(target_copy, "/");

		// Special handling for /view: the filename must be a blob ID.
		// If the length of the filename is not equal to the length of blob IDs (i.e. 40 byte hex)
		// then treat it as collection instead.
		if (m_action != Action::view || is_valid_blob_id(filename))
		{
			m_filename = url_decode(filename);
			target = target_copy;
		}
	}

	if (target.empty())
		return;

	m_coll = url_decode(trim(target));
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
		case Action::view:      oss << "view/";     break;
		case Action::list:      oss << "list/";     break;
		case Action::upload:    oss << "upload/";   break;
		case Action::lib:       oss << "lib/";      break;
		case Action::listcolls: oss << "listcolls/";    break;

		case Action::home:
		case Action::none:
			break;
	}
	oss << url_encode(m_user);
	if (!m_user.empty())
		oss << '/';
	oss << url_encode(m_coll);
	if (!m_coll.empty())
		oss << '/';
	oss << url_encode(m_filename);
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
	     if (str == "view")     return Action::view;
	else if (str == "list")     return Action::list;
	else if (str == "upload")   return Action::upload;
	else if (str == "login")    return Action::login;
	else if (str == "logout")   return Action::logout;
	else if (str == "lib")      return Action::lib;
	else if (str == "query")    return Action::query;
	else if (str == "listcolls")    return Action::listcolls;
	else if (str.empty())       return Action::home;
	else                        return Action::none;
}

const std::array<bool, static_cast<int>(URLIntent::Action::none)> URLIntent::require_user =
//   login, logout, view, coll, upload, home,  lib,   listcolls, query, none
	{false, false,  true, true, true,   false, false, true,      false};
const std::array<bool, static_cast<int>(URLIntent::Action::none)> URLIntent::forbid_user =
//   login, logout, view,  coll,  upload, home, lib,  listcolls, query, none
	{true,  true,   false, false, false,  true, true, false,     true};

const std::array<bool, static_cast<int>(URLIntent::Action::none)> URLIntent::require_filename =
//   login, logout, view,  coll,  upload, home,  lib,  listcolls, query, none
	{false, false,  false, false, true,   false, true, false,     true};
const std::array<bool, static_cast<int>(URLIntent::Action::none)> URLIntent::forbid_filename =
//   login, logout, view,  coll, upload, home, lib,   listcolls, query, none
	{true,  true,   false, true,  false,  true, false, true,      false};

const std::array<bool, static_cast<int>(URLIntent::Action::none)> URLIntent::forbid_coll =
//   login, logout, view,  coll,  upload, home, lib,  listcolls, query, none
	{true,  true,   false, false, false,  true, true, false,     false};

bool URLIntent::valid() const
{
	if (m_action != Action::none)
	{
		auto action_index = static_cast<std::size_t>(m_action);
		assert(!require_user.at(action_index)     || !forbid_user.at(action_index));
		assert(!require_filename.at(action_index) || !forbid_filename.at(action_index));
		assert(require_user.at(static_cast<std::size_t>(Action::view)));

		if (require_user.at(action_index) && m_user.empty())
			return false;

		if (forbid_user.at(action_index) && !m_user.empty())
			return false;

		if (require_filename.at(action_index) && m_filename.empty())
			return false;

		if (forbid_filename.at(action_index) && !m_filename.empty())
			return false;

		if (forbid_coll.at(action_index) && !m_coll.empty())
			return false;

		return true;
	}
	else
	{
		return false;
	}
}

bool URLIntent::need_auth() const
{
	return m_action == Action::upload;
}

} // end of namespace hrb
