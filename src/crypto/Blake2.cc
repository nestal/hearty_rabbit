/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/28/18.
//

#include "Blake2.hh"

#include <cstring>
#include <cassert>

namespace hrb {

Blake2::Blake2()
{
	::blake2b_init(&m_ctx, size);
}

void Blake2::update(const void *data, std::size_t len)
{
	::blake2b_update(&m_ctx, static_cast<const std::uint8_t*>(data), len);
}

std::size_t Blake2::finalize(unsigned char *out, std::size_t len)
{
	::blake2b_final(&m_ctx, out, len);
	return len;
}

Blake2::Hash Blake2::finalize()
{
	std::array<unsigned char, Blake2::size> result{};
	finalize(&result[0], result.size());
	return result;
}

} // end of namespace hrb
