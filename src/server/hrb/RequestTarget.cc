/*
	Copyright © 2023 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 7/10/23.
//

#include "RequestTarget.hh"
#include "util/Escape.hh"

namespace hrb {

RequestTarget::RequestTarget(std::string_view target)
{
	// Extract the query string:
	// only truncate "target" when "?" is found; keep "target" unchanged if "?" is not found
	// use split_left() because the query string starts from the _first_ '?' according to
	// [RFC 3986](https://tools.ietf.org/html/rfc3986#page-23).
	auto tmp = target;
	auto[field, sep] = split_left(tmp, "?");
	if (sep == '?')
	{
		m_action = tmp;
		target   = field;
	}

	m_path = target.empty() ? "/" : target;
}

} // end of namespace hrb