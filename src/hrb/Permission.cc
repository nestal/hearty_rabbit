/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 3/8/18.
//

#include "Permission.hh"

namespace hrb {

Permission::Permission(std::string_view str) :
	m_str{str}
{
}

bool Permission::allow(std::string_view user)
{
	// assume not owner. no need to call this if owner.
	if (m_str.empty())
		return false;

	switch (m_str.front())
	{
		case '*':   return true;
		case '+':   return !user.empty();

		// TODO: handle ACL
		default:    return false;
	}
}


} // end of namespace hrb
