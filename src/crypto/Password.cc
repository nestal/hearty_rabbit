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

#include <cstring>
#include <system_error>

#include <sys/mman.h>

namespace hrb {

Password::Password(std::string_view val)
{
	if (!val.empty())
	{
		m_val.resize(val.size());
		if (::mlock(&m_val[0], m_val.size()) != 0)
			throw std::system_error(errno, std::generic_category());

		// lock before copy
		std::copy(val.begin(), val.end(), m_val.begin());
	}
}

Password::~Password()
{
	clear();
}

void Password::swap(Password& other)
{
	m_val.swap(other.m_val);
}

Password::Key Password::derive_key(std::string_view salt, int iteration) const
{
	Key key{};
	if (::PKCS5_PBKDF2_HMAC(
		m_val.empty() ? nullptr : &m_val[0],
		static_cast<int>(m_val.size()),
		reinterpret_cast<const unsigned char*>(salt.data()),
		static_cast<int>(salt.size()),
		iteration,
		::EVP_sha512(),
		static_cast<int>(key.size()),
		&key[0]
	) != 1)
		throw std::system_error(errno, std::generic_category());

	return key;
}

std::string_view Password::get() const
{
	return {&m_val[0], m_val.size()};
}

void Password::clear()
{
	if (!m_val.empty())
	{
		::memset(&m_val[0], 0, m_val.size());
		::munlock(&m_val[0], m_val.size());
		m_val.clear();
	}
}

bool Password::empty() const
{
	return m_val.empty();
}

std::size_t Password::size() const
{
	return m_val.size();
}

} // end of namespace hrb
