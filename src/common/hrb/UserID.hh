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

#include <chrono>
#include <array>
#include <optional>
#include <string>

namespace hrb {

class Cookie;

class UserID
{
public:
	using SessionID = std::array<unsigned char, 16>;

public:
	UserID() = default;
	UserID(SessionID session, std::string_view user, bool guest=false);
	UserID(const Cookie& cookie, std::string_view user);

	const SessionID& session() const {return m_session;}
	const std::string& username() const {return m_user;}
	bool is_guest() const {return m_guest;}

	bool valid() const;

	bool operator==(const UserID& rhs) const;
	bool operator!=(const UserID& rhs) const;

	Cookie cookie() const;
	Cookie set_cookie(std::chrono::seconds session_length = std::chrono::seconds{3600}) const;

	static std::optional<SessionID> parse_cookie(const Cookie& cookie);

private:
	SessionID       m_session{};
	std::string     m_user;
	bool            m_guest{false};
};

} // end of namespace hrb
