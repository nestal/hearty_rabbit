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

namespace hrb {

void secure_random(void *buf, std::size_t size)
{
	if (::getrandom(buf, size, 0) != size)
		throw std::system_error(errno, std::generic_category());
}

// Blake2x random number generator: https://blake2.net/blake2x.pdf
Blake2x::Blake2x()
{
	m_param.digest_length = 8;
	m_param.key_length    = 0;
	m_param.fanout        = 0;
	m_param.depth         = 0;
	m_param.leaf_length   = 64;
	m_param.node_depth    = 0;
	m_param.inner_length  = 64;
	m_param.node_offset   = boost::endian::native_to_little(UINT64_C(0xffffffff00000000));

	// leave reserved as zero
	secure_random(m_param.salt, sizeof(m_param.salt));
	// leave personalization as zero
}

Blake2x::result_type Blake2x::operator()()
{
	::blake2b_state ctx{};
	if (::blake2b_init_param(&ctx, &m_param) < 0)
		throw std::runtime_error("blake2b init error");
	if (::blake2b_update(&ctx, reinterpret_cast<std::uint8_t*>(&m_h0), sizeof(m_h0)) < 0)
		throw std::runtime_error("blake2b update error");

	result_type hash{};
	if (::blake2b_final(&ctx, reinterpret_cast<std::uint8_t*>(&hash), sizeof(hash)) < 0)
		throw std::runtime_error("blake2b final error");

	// Update generator state
	auto native_offset = boost::endian::little_to_native(m_param.node_offset);
	assert(native_offset >> 32 == 0xffffffff);
	native_offset++;
	m_param.node_offset = boost::endian::native_to_little(native_offset);

	return hash;
}

} // end of namespace hrb
