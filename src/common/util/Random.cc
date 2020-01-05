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
Blake2x::Blake2x() noexcept
{
	m_param.digest_length = 8;
	m_param.key_length = 0;
	m_param.fanout = 0;
	m_param.depth = 0;
	m_param.leaf_length = 64;
	m_param.node_depth = 0;
	m_param.inner_length = 64;
	m_param.node_offset = boost::endian::native_to_little(UINT64_C(0xffffffff00000000));
	// leave reserved as zero
	// leave personalization as zero

	reseed();
}

Blake2x::result_type Blake2x::operator()()
{
	::blake2b_state ctx{};
	::blake2b_init_param(&ctx, &m_param);
	::blake2b_update(&ctx, reinterpret_cast<std::uint8_t*>(&m_h0), sizeof(m_h0));

	result_type hash{};
	::blake2b_final(&ctx, reinterpret_cast<std::uint8_t*>(&hash), sizeof(hash));

	// Update generator state
	auto native_offset = boost::endian::little_to_native(m_param.node_offset);
	assert(native_offset >> 32U == 0xffffffffU);
	native_offset++;
	m_param.node_offset = boost::endian::native_to_little(native_offset);

	// Technically we can go up to 2^32 blocks before reseeding,
	// but reseeding more often is safer.
	if ((native_offset & 0xffffffff) % 0x7ffff == 0)
		reseed();

	return hash;
}

Blake2x& Blake2x::instance()
{
	thread_local Blake2x inst;
	return inst;
}

void Blake2x::reseed()
{
	reseed(SecureRandom<std::uint64_t>{});
}

} // end of namespace hrb
