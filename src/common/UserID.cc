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
#include "Cookie.hh"
#include "Escape.hh"

#include <cassert>
#include <iostream>

namespace hrb {

UserID::UserID(SessionID session, std::string_view user, bool guest) :
	m_session{session}, m_user{user}, m_guest{guest}
{
	if (m_user.empty())
		m_session = SessionID{};
}

UserID::UserID(const Cookie& cookie, std::string_view user) :
	m_session{hex_to_array<SessionID{}.size()>(cookie.field("id")).value_or(SessionID{})},
	m_user{user}
{
}

bool UserID::valid() const
{
	// if cookie is valid, user must not be empty
	// if cookie is invalid, user must be empty
	assert((m_session == SessionID{}) == m_user.empty() );

	return m_session != SessionID{} && !m_user.empty();
}


bool UserID::operator==(const UserID& rhs) const
{
	return m_user == rhs.m_user && m_session == rhs.m_session && m_guest == rhs.m_guest;
}

bool UserID::operator!=(const UserID& rhs) const
{
	return !operator==(rhs);
}

Cookie UserID::cookie() const
{
	Cookie cookie;
	cookie.add("id", valid() ? to_hex(m_session) : "");
	return cookie;
}

std::string UserID::set_cookie(std::chrono::seconds session_length) const
{
	std::string result{cookie().str()};
	if (valid())
	{
		result.append("; Secure; HttpOnly; SameSite=Strict; Path=/; Max-Age=");
		result.append(std::to_string(session_length.count()));
	}
	else
	{
		result.append("; Path=/; expires=Thu, Jan 01 1970 00:00:00 UTC");
	}
	return result;
}

} // end of namespace hrb
