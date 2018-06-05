/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 6/5/18.
//

#include <iostream>
#include "Cookie.hh"
#include "StringFields.hh"

namespace hrb {
namespace {
const StringFields fields{"=;", ";"};
}

Cookie::Cookie(std::string_view header) : m_cookie{header}
{
}

std::chrono::system_clock::time_point Cookie::expires() const
{
	auto [exp_str] = fields.find(m_cookie, " Expires");
	std::cout << "Expires = " << exp_str << std::endl;

	return std::chrono::system_clock::time_point();
}

} // end of namespace hrb
