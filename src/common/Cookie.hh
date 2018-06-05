/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 6/5/18.
//

#pragma once

#include <string>
#include <optional>
#include <chrono>

namespace hrb {

/// \brief  Encapsulate a list of cookies inside a HTTP header
/// This class is used to parse the Set-Cookie and Cookie HTTP header and
/// store the result.
///
/// See https://www.ietf.org/rfc/rfc2109.txt for reference.
class Cookie
{
public:
	explicit Cookie(std::string_view header = {});

	Cookie(const Cookie&) = default;
	Cookie(Cookie&&) = default;
	Cookie& operator=(const Cookie&) = default;
	Cookie& operator=(Cookie&&) = default;

	std::chrono::system_clock::time_point expires() const;
	std::string_view field(std::string_view id) const;

	void add(std::string_view id, std::string_view value);
	bool has(std::string_view id) const;

private:
	std::string m_cookie;   //!< the value of the header field "Set-Cookie" or "Cookie"

	// cached parse results
	mutable std::optional<std::chrono::system_clock::time_point> m_expires;
};

} // end of namespace hrb
