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
	::EVP_DigestInit_ex(m_ctx.get(), ::EVP_blake2s256(), nullptr);
}

void Blake2::update(const void *data, std::size_t len)
{
	::EVP_DigestUpdate(m_ctx.get(), data, len);
}

std::size_t Blake2::finalize(unsigned char *out, std::size_t len)
{
	auto ilen = static_cast<unsigned>(len);
	::EVP_DigestFinal_ex(m_ctx.get(), out, &ilen);
	return ilen;
}

std::array<unsigned char, Blake2::size> Blake2::finalize()
{
	std::array<unsigned char, Blake2::size> result{};
	finalize(&result[0], result.size());
	return result;
}

void Blake2::Deleter::operator()(::EVP_MD_CTX *ctx)
{
	if (ctx)
		::EVP_MD_CTX_free(ctx);
}

} // end of namespace hrb
