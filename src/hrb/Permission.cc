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

Permission::Permission(char perm) : m_perm{perm}
{
}

bool Permission::allow(std::string_view user)
{
	switch (m_perm)
	{
		case '*':   return true;
		case '+':   return !user.empty();
		case ' ':	return false;

		// TODO: handle ACL
		default:    return false;
	}
}

Permission Permission::shared()
{
	return Permission{'+'};
}

Permission Permission::public_()
{
	return Permission{'*'};
}

Permission Permission::private_()
{
	return Permission{' '};
}

std::string_view Permission::description() const
{
	switch (m_perm)
	{
		case '*':   return "public";
		case '+':   return "shared";
		case ' ':	return "private";
		default:    return "acl";
	}
}

Permission Permission::from_description(std::string_view description)
{
	if (description == "public")
		return Permission::public_();
	else if (description == "private")
		return Permission::public_();
	else if (description == "shared")
		return Permission::shared();
	else
		// default is private
		return Permission{};
}

} // end of namespace hrb
