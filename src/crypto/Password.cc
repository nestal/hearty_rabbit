/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/27/18.
//

#include "Password.hh"

#include <openssl/evp.h>
#include <openssl/sha.h>

#include <sys/mman.h>
#include <cstring>

namespace hrb {

Password::Password(std::string_view val)
{
	if (!val.empty())
	{
		m_val.assign(val.begin(), val.end());
		::munlock(&m_val[0], m_val.size());
	}
}

Password::~Password()
{
	if (!m_val.empty())
	{
		::memset(&m_val[0], 0, m_val.size());
		::munlock(&m_val[0], m_val.size());
	}
}

void Password::swap(Password& other)
{
	m_val.swap(other.m_val);
}

Password::Key Password::derive_key(std::string_view salt, int iteration) const
{
	Key key{};
	::PKCS5_PBKDF2_HMAC(
		&m_val[0],
		static_cast<int>(m_val.size()),
		reinterpret_cast<const unsigned char*>(salt.data()),
		static_cast<int>(salt.size()),
		5000,
		::EVP_sha512(),
		static_cast<int>(key.size()),
		&key[0]
	);
	return key;
}

std::string_view Password::get() const
{
	return {&m_val[0], m_val.size()};
}

} // end of namespace hrb
