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

Password::Password(Password&& other)
{
	swap(other);
}

Password::~Password()
{
	if (m_mem)
	{
		::memset(m_mem, 0, m_size);
		::free(m_mem);
	}
}

Password::Password(std::string_view val)
{
	if (!val.empty())
	{
		m_mem = static_cast<char*>(::malloc(val.size()));
		if (!m_mem)
			throw std::bad_alloc();

		m_size = val.size();
		::mlock(m_mem, m_size);
		::memcpy(m_mem, val.data(), val.size());
	}
}

Password& Password::operator=(Password&& other)
{
	Password tmp{std::move(other)};
	swap(tmp);
	return *this;
}

void Password::swap(Password& other)
{
	std::swap(m_mem, other.m_mem);
	std::swap(m_size, other.m_size);
}

Password::Key Password::derive_key(std::string_view salt, int iteration) const
{
	Key key{};
	::PKCS5_PBKDF2_HMAC(
		m_mem,
		static_cast<int>(m_size),
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
	return {static_cast<const char*>(m_mem), m_size};
}

} // end of namespace hrb
