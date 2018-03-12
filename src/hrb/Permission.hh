/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 3/8/18.
//

#pragma once

#include <string_view>
#include <string>

namespace hrb {

class Permission
{
public:
	Permission(std::string_view str);

	bool allow(std::string_view user);

private:
	std::string m_str;
};

} // end of namespace hrb
