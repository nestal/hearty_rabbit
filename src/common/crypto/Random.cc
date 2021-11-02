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

#include <cassert>
#include <limits>
#include <system_error>

#include <openssl/rand.h>
#include <openssl/err.h>

// C++17 is doing cmake's job
#if __has_include(<sys/random.h>)
#include <sys/random.h>
#else
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/random.h>

namespace {

// The glibc in CentOS 7 does not have the getrandom() wrapper yet, but its kernel does
// support the getrandom() system call.
ssize_t getrandom(void *buf, size_t size, unsigned int flags)
{
	return syscall(SYS_getrandom, buf, size, flags);
}

} // end of local namespace
#endif

namespace hrb::detail {

void system_random(void *buf, std::size_t size)
{
	if (::getrandom(buf, size, 0) != size)
		throw std::system_error(errno, std::generic_category());
}

void user_random(void *buf, std::size_t size)
{
	assert(size < static_cast<std::size_t>(std::numeric_limits<int>::max()));
	if (::RAND_bytes(reinterpret_cast<unsigned char*>(buf), static_cast<int>(size)) != 1)
	{
		// according to OpenSSL man page, 256 bytes is enough.
		// https://www.openssl.org/docs/manmaster/man3/ERR_error_string.html
		char error_string[256];
		::ERR_error_string_n(::ERR_get_error(), error_string, sizeof(error_string));

		throw std::runtime_error(error_string);
	}
}

} // end of namespace hrb
