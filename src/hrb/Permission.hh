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

namespace hrb {

class Permission
{
public:
	Permission() = default;
	explicit Permission(char perm);

	static Permission from_description(std::string_view description);

	static Permission shared();
	static Permission public_();
	static Permission private_();

	bool allow(std::string_view user);

	std::string_view str() const {return {&m_perm, 1};}
	std::string_view description() const;

	constexpr const char* data() const {return &m_perm;}
	constexpr std::size_t size() const {return 1;}
	constexpr char perm() const {return m_perm;}

	bool operator==(const Permission& other) const;
	bool operator!=(const Permission& other) const;

private:
	char m_perm{' '};
};

} // end of namespace hrb
