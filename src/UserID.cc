/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 6/3/18.
//

#include "UserID.hh"

namespace hrb {

UserID::UserID(std::string_view user) :
	m_user{user}, m_guest{false}
{
}

bool UserID::operator==(const UserID& rhs) const
{
	return m_user == rhs.m_user && m_guest == rhs.m_guest;
}

bool UserID::operator!=(const UserID& rhs) const
{
	return !operator==(rhs);
}

} // end of namespace hrb
