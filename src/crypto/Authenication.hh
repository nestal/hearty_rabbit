/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/27/18.
//

#pragma once

#include <array>
#include <functional>
#include <optional>
#include <string_view>
#include <system_error>

namespace hrb {

namespace redis {
class Connection;
}

class Password;
using SessionID = std::array<unsigned char, 16>;

void add_user(
	std::string_view username,
	const Password& password,
	redis::Connection& db,
	std::function<void(std::error_code)> completion
);

void verify_user(
	std::string_view username,
	Password&& password,
	redis::Connection& db,
	std::function<void(std::error_code, const SessionID&)> completion
);

void verify_session(
	const SessionID& id,
	redis::Connection& db,
	std::function<void(std::error_code, std::string_view user)> completion
);

std::string set_cookie(const SessionID& id);
std::optional<SessionID> parse_cookie(std::string_view cookie);

} // end of namespace
