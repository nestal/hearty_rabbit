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

#include <optional>
#include <string>

namespace hrb {

/// There are three types of users: anonymous, guests and valid users.
/// Anonymous users send requests without any session ID in cookies or authentication keys in URL.
/// Guest users send requests without session ID in cookies, but they have a valid authentication key in all
/// URLs. Valid users send requests with a valid session ID, and we don't care if they have any authentication
/// keys in URL.
class UserID
{
public:
	UserID() = default;
	explicit UserID(std::string_view user);
	UserID(const UserID&) = default;
	UserID(UserID&&) = default;
	~UserID() = default;

	UserID& operator=(const UserID&) = default;
	UserID& operator=(UserID&&) = default;

	[[nodiscard]] const std::string& username() const {return m_user;}
	[[nodiscard]] bool is_guest() const {return m_guest;}

	// Returns true only when this object represent a valid user.
	// Unlike Authentication::is_valid(), UserID::is_valid() return false if it represents a guest user.
	[[nodiscard]] bool is_valid() const {return !m_user.empty();}

	[[nodiscard]] bool is_anonymous() const {return m_user.empty() && !m_guest;}

	bool operator==(const UserID& rhs) const;
	bool operator!=(const UserID& rhs) const;

	static UserID guest()
	{
		UserID guest;
		guest.m_guest = true;
		return guest;
	}

private:
	std::string     m_user;
	bool            m_guest{false};
};

} // end of namespace hrb
