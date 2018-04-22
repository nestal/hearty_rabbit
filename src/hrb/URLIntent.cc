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

	// query
	Parameters{URLIntent::Parameter::query_target, URLIntent::Parameter::option},

	// api
	Parameters{
		URLIntent::Parameter::user,
		URLIntent::Parameter::collection,
		URLIntent::Parameter::blob,
		URLIntent::Parameter::option
	}
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

	// assume valid unless see unwanted fields
	m_valid = true;

	auto& intent_definition = intent_defintions[static_cast<std::size_t>(m_action)];
	auto mid = std::find_first_of(intent_definition.begin(), intent_definition.end(), separator_fields.begin(), separator_fields.end());
	for (auto i = intent_definition.begin() ; i != mid; i++)
		parse_field_from_left(target, *i);
	for (auto i = intent_definition.rbegin() ; i != std::reverse_iterator<Parameters::const_iterator>{mid}; i++)
		parse_field_from_right(target, *i);;

	if (!target.empty())
		m_valid = false;
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
		case Parameter::user:         m_user     = url_decode(field); break;
		case Parameter::filename:     m_filename = url_decode(field); break;
		case Parameter::query_target: m_query_target = parse_query_target(field); break;
		default: break;
	}
}

void URLIntent::parse_field_from_right(std::string_view& target, hrb::URLIntent::Parameter p)
{
	if (p == Parameter::option)
	{
		// only truncate "target" when "?" is found; keep "target" unchange if "?" is not found
		// use split_left() because the query string starts from the _first_ '?' according to
		// [RFC 3986](https://tools.ietf.org/html/rfc3986#page-23).
		auto tmp = target;
		auto[field, sep] = split_left(tmp, "?");
		if (sep == '?')
		{
			m_option = tmp;
			target   = field;
		}

		if (m_action == Action::query && m_option.empty())
			m_valid = false;
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
		case Action::query:     oss << "query/";    break;
		case Action::api:       oss << "api/";      break;

		case Action::home:
		case Action::none:
			break;
	}

	auto& intent_definition = intent_defintions[static_cast<std::size_t>(m_action)];
	for (auto&& intent : intent_definition)
	{
		switch (intent)
		{
		case Parameter::user:
			oss << url_encode(m_user);
			if (!m_user.empty())
				oss << '/';
			break;

		case Parameter::collection:
			oss << url_encode(m_coll);
			if (!m_coll.empty())
				oss << '/';
			break;

		case Parameter::filename:
			oss << url_encode(m_filename);
			break;

		case Parameter::blob:
			oss << m_filename;
			break;

		case Parameter::option:
			if (!m_option.empty())
				oss << "?" << m_option;
			break;

		case Parameter::query_target:
			oss << to_string(m_query_target);
			break;
		}
	}

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
	else if (str == "api")      return Action::api;
	else if (str.empty())       return Action::home;
	else                        return Action::none;
}

bool URLIntent::valid() const
{
	if (m_action != Action::none)
	{
		auto action_index = static_cast<std::size_t>(m_action);
		static_assert(require_user.at(static_cast<std::size_t>(Action::view)));

		if (require_user.at(action_index) && m_user.empty())
			return false;

		if (require_filename.at(action_index) && m_filename.empty())
			return false;

		return m_valid;
	}
	else
	{
		return false;
	}
}

URLIntent::QueryTarget URLIntent::parse_query_target(std::string_view str)
{
	     if (str == "blob")       return QueryTarget::blob;
	else if (str == "collection") return QueryTarget::collection;
	else if (str == "blob_set")   return QueryTarget::blob_set;
	else                          return QueryTarget::none;
}

std::string_view URLIntent::to_string(URLIntent::QueryTarget query_target)
{
	switch (query_target)
	{
		case QueryTarget::blob:         return "blob";
		case QueryTarget::collection:   return "collection";
		case QueryTarget::blob_set:     return "blob_set";
		default: return "";
	}
}

} // end of namespace hrb
