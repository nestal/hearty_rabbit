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

#include <string_view>
#include <functional>
#include <system_error>

namespace hrb {

namespace redis {
class Database;
}

class Authenication
{
public:
	Authenication(std::string_view username, std::string_view password, redis::Database& db);

private:
	std::string m_cookie;
};

void add_user(
	std::string_view username,
	std::string_view password,
	redis::Database& db,
	std::function<void(std::string_view cookie, std::error_code)> completion
);

} // end of namespace
