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

#include <openssl/sha.h>
#include <array>

namespace hrb {

class Password
{
public:
	Password() = default;
	Password(Password&& other);
	Password(const Password&) = delete;
	explicit Password(std::string_view val);
	~Password();

	Password& operator=(Password&& other);
	Password& operator=(const Password&) = delete;
	void swap(Password& other);

	using Key = std::array<unsigned char, SHA512_DIGEST_LENGTH>;
	Key derive_key(std::string_view salt, int iteration) const;
	std::string_view get() const;

private:
	char *m_mem{};
	std::size_t m_size{};
};

} // end of namespace hrb
