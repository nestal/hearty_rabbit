/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/28/18.
//

#include "SHA2.hh"

#include <cstring>
#include <cassert>

namespace hrb {
namespace evp {

SHA2::SHA2()
{
	::blake2b_init(&m_ctx, size);
}

void SHA2::update(const void *data, std::size_t len)
{
	::blake2b_update(&m_ctx, static_cast<const std::uint8_t*>(data), len);
}

std::size_t SHA2::finalize(unsigned char *out, std::size_t len)
{
	::blake2b_final(&m_ctx, out, len);
	return len;
}

std::array<unsigned char, SHA2::size> SHA2::finalize()
{
	std::array<unsigned char, SHA2::size> result{};
	finalize(&result[0], result.size());
	return result;
}

}} // end of namespace hrb
