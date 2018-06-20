/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 6/3/18.
//

#pragma once

#include <array>

namespace hrb {

class UserID
{
public:
	using CookieID = std::array<unsigned char, 16>;

public:
	UserID() = default;
	UserID(CookieID cookie, std::string_view user, bool guest=false);

	const CookieID& cookie() const {return m_cookie;}
	const std::string& user() const {return m_user;}
	bool is_guest() const {return m_guest;}

	bool valid() const;

	bool operator==(const UserID& rhs) const;
	bool operator!=(const UserID& rhs) const;

private:
	CookieID      m_cookie{};
	std::string m_user;
	bool        m_guest{false};
};

} // end of namespace hrb
