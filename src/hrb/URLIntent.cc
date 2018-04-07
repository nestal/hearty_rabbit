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

	// upload
	Parameters{URLIntent::Parameter::user, URLIntent::Parameter::collection, URLIntent::Parameter::filename},

	// home
	Parameters{URLIntent::Parameter::option},

	// lib
	Parameters{URLIntent::Parameter::filename},

	// listcolls
	Parameters{URLIntent::Parameter::user},

	// query
	Parameters{URLIntent::Parameter::command, URLIntent::Parameter::filename, URLIntent::Parameter::option}
};

const URLIntent::Parameters URLIntent::separator_fields{URLIntent::Parameter::collection, URLIntent::Parameter::option};

namespace {
std::string_view extract_left(std::string_view& target)
{
	auto tmp = target;
	auto[field, sep] = split_left(tmp, "/?#");
	if (sep == '/')
		target = tmp;
	else
		target.remove_prefix(field.size());
	return field;
}
} // end of local namespace

URLIntent::URLIntent(boost::string_view boost_target)
{
	// order is important here
	// All URL path must start with slash
	if (boost_target.empty() || boost_target.front() != '/')
		return;

	// skip the first slash
	std::string_view target{boost_target.data(), boost_target.size()};
	target.remove_prefix(1);

	// extract the action, and parse the rest according to the action
	m_action = parse_action(extract_left(target));
	if (m_action == Action::none)
		return;

	auto& intent_definition = intent_defintions[static_cast<std::size_t>(m_action)];
	auto mid = std::find_first_of(intent_definition.begin(), intent_definition.end(), separator_fields.begin(), separator_fields.end());
	for (auto i = intent_definition.begin() ; i != mid; i++)
		parse_field_from_left(target, *i);
	for (auto i = intent_definition.rbegin() ; i != std::reverse_iterator<Parameters::const_iterator>{mid}; i++)
		parse_field_from_right(target, *i);;

	m_valid = target.empty();
}

URLIntent::URLIntent(Action act, std::string_view user, std::string_view coll, std::string_view name) :
	m_action{act}, m_user{trim(user)}, m_coll{trim(coll)}, m_filename{trim(name)}, m_valid{true}
{
}


void URLIntent::parse_field_from_left(std::string_view& target, hrb::URLIntent::Parameter p)
{
	assert(p != Parameter::collection && p != Parameter::blob);
	auto field = extract_left(target);

	switch (p)
	{
		case Parameter::user:       m_user     = url_decode(field); break;
		case Parameter::filename:   m_filename = url_decode(field); break;
		case Parameter::command:    m_command  = field; break;
		default: break;
	}
}

void URLIntent::parse_field_from_right(std::string_view& target, hrb::URLIntent::Parameter p)
{
	if (p == Parameter::option)
	{
		// only truncate "target" when "?" is found
		// keep "target" unchange if "?" is not found
		auto tmp = target;
		auto[field, sep] = split_right(tmp, "?");
		if (sep == '?')
		{
			m_option = field;
			target   = tmp;
		}
	}
	else if (p == Parameter::filename || p == Parameter::blob)
	{
		// After split_right() remove the matched string from "target", we can't undo it.
		// Therefore we make a copy of target and match that instead.
		auto target_copy = target;
		auto [field, sep] = split_right(target_copy, "/");

		// Special handling for Parameter::blob: the filename must be a blob ID.
		if (p == Parameter::filename || is_valid_blob_id(field))
		{
			// Here we want to commit the changes to "target", only when
			// the "filename" is a valid blob ID.
			m_filename = url_decode(field);
			target     = target_copy;
		}
	}
	else if (p == Parameter::collection)
	{
		m_coll = url_decode(trim(target));
		target = std::string_view{};
	}
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
//   login, logout, view, upload, home,  lib,   listcolls, query, none
	{false, false,  true, true,   false, false, true,      false};
const std::array<bool, static_cast<int>(URLIntent::Action::none)> URLIntent::forbid_user =
//   login, logout, view,  upload, home, lib,  listcolls, query, none
	{true,  true,   false, false,  true, true, false,     true};

const std::array<bool, static_cast<int>(URLIntent::Action::none)> URLIntent::require_filename =
//   login, logout, view,  upload, home,  lib,  listcolls, query, none
	{false, false,  false, true,   false, true, false,     false};
const std::array<bool, static_cast<int>(URLIntent::Action::none)> URLIntent::forbid_filename =
//   login, logout, view,  upload, home, lib,   listcolls, query, none
	{true,  true,   false, false,  true, false, true,      false};

const std::array<bool, static_cast<int>(URLIntent::Action::none)> URLIntent::forbid_coll =
//   login, logout, view,  upload, home, lib,  listcolls, query, none
	{true,  true,   false, false,  true, true, false,     true};

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

		return m_valid;
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
