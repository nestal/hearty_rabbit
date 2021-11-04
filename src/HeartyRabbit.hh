/*
	Copyright © 2021 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/11/2021.
//

#pragma once

#include "crypto/Authentication.hh"

#include <string_view>
#include <filesystem>

namespace hrb {

class Password;
class Cookie;

class HeartyRabbit
{
public:
	explicit HeartyRabbit(std::filesystem::path root);

	void login(
		std::string_view user_name, const Password& password
	);

private:
	Authentication  m_user;
};

} // end of namespace hrb
