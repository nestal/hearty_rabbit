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

#include "ObjectID.hh"
#include "util/Escape.hh"

#include <sstream>
#include <cassert>

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

	// user URLs
	if (first.front() == '~')
	{
		User user{};
		first.remove_prefix(1);
		user.username = first;

		m_var.emplace<User>(user);
	}
}

} // end of namespace hrb
