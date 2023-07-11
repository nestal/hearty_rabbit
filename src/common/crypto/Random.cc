/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/27/18.
//

#include "Random.hh"

#include <boost/endian/conversion.hpp>

#include <system_error>
#include <utility>
#include <cassert>
#include <limits>

#include <openssl/rand.h>
#include <openssl/err.h>

namespace {
template <typename OpenSSLRandomFunction>
inline void open_ssl_rand(void *buf, std::size_t size, OpenSSLRandomFunction&& func)
{
	assert(size <= std::numeric_limits<int>::max());

	// TODO: better error handling
	if (func(reinterpret_cast<unsigned char*>(buf), static_cast<int>(size)) != 1)
		throw std::system_error(static_cast<int>(::ERR_get_error()), std::generic_category());
}
}

namespace hrb {
void secure_random(void *buf, std::size_t size)
{
	open_ssl_rand(buf, size, ::RAND_priv_bytes);
}

void insecure_random(void *buf, std::size_t size)
{
	open_ssl_rand(buf, size, ::RAND_bytes);
}

} // end of namespace hrb
