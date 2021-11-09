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
#include "util/Overload.hh"
#include "util/StringFields.hh"

#include <sstream>
#include <cassert>
#include <iostream>

namespace hrb {
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

URLIntent::URLIntent(std::string_view target)
{
	// order is important here
	// All URL path must start with slash
	if (target.empty() || target.front() != '/')
		return;

	// skip the first slash
	target.remove_prefix(1);

	auto first = extract_left(target);
	if (first.empty())
		return;

	// Extract the query string:
	// only truncate "target" when "?" is found; keep "target" unchange if "?" is not found
	// use split_left() because the query string starts from the _first_ '?' according to
	// [RFC 3986](https://tools.ietf.org/html/rfc3986#page-23).
	auto query = target;
	auto[mid, sep] = split_left(query, "?");

	// user URLs
	if (first.front() == '~')
	{
		first.remove_prefix(1);
		User user{.username=std::string{first}};

		if (sep == '?')
			std::tie(user.rendition) = urlform.find(query, "rend");

		user.path.append("/").append(mid);
		m_var.emplace<User>(user);
	}
	else if (first == "session" && sep == '?')
	{
		if (query == "create")
			m_var.emplace<Session>(Session{Session::Action::create});
		else if (query == "destroy")
			m_var.emplace<Session>(Session{Session::Action::destroy});
	}
}

std::string URLIntent::str() const
{
	using namespace std::string_literals;
	return visit(
		Overloaded{
			[](const Session& s)
			{
				return "/session?"s.append(s.action == Session::Action::create ? "create" : "destroy");
			},
			[](const User& u)
			{
				assert(u.path.is_absolute());
				auto str = "/~"s.append(u.username).append(u.path.string());
				if (!u.rendition.empty())
					str.append("?rend=").append(u.rendition);
				return str;
			},
			[](const Query&){return ""s;},
			[](const Lib&){return ""s;},
			[](auto&&){return "/"s;}
		},
		m_var
	);
}

} // end of namespace hrb
