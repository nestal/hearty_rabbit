/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 6/5/18.
//

#include "Cookie.hh"
#include "StringFields.hh"

#include <iostream>
#include <sstream>
#include <locale>
#include <iomanip>

namespace hrb {
namespace {
const StringFields fields{"=;", ";", true};
}

Cookie::Cookie(std::string_view header) : m_cookie{header}
{
}

std::chrono::system_clock::time_point Cookie::expires() const
{
	if (!m_expires.has_value())
	{
		auto[exp_str] = fields.find(m_cookie, "Expires");
		std::cout << "Expires = " << exp_str << std::endl;

		std::istringstream ss{std::string{exp_str}};
		std::tm tm{};

		// Wed, 21 Oct 2015 07:28:00 GMT
		if (ss >> std::get_time(&tm, "%a, %d %b %Y %H:%M:%S"))
			m_expires = std::chrono::system_clock::from_time_t(::timegm(&tm));
	}
	return *m_expires;
}

std::string_view Cookie::field(std::string_view id) const
{
	return std::get<0>(fields.find(m_cookie, id));
}

void Cookie::add(std::string_view id, std::string_view value)
{
	m_cookie += ";";
	m_cookie += id;
	m_cookie += "=";
	m_cookie += value;
}

bool Cookie::has(std::string_view id) const
{
	return std::get<0>(fields.find_optional(m_cookie, id)).has_value();
}

} // end of namespace hrb
