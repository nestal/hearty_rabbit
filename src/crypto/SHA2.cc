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

#if OPENSSL_VERSION_NUMBER < 0x1010000fL
// Ployfill
void EVP_MD_CTX_destroy(EVP_MD_CTX *ctx)
{
	EVP_MD_CTX_cleanup(ctx);
	delete ctx;
}
EVP_MD_CTX* EVP_MD_CTX_new()
{
	auto ctx = new EVP_MD_CTX;
	EVP_MD_CTX_init(ctx);
	return ctx;
}
#endif

void SHA2::Deleter::operator()(EVP_MD_CTX *ctx) const
{
	// This is a macro, so we can't take its address and put it to unique_ptr.
	::EVP_MD_CTX_destroy(ctx);
}

SHA2::SHA2() : m_ctx{EVP_MD_CTX_new(), Deleter{}}
{
	::EVP_DigestInit_ex(m_ctx.get(), ::EVP_sha256(), nullptr);
}

void SHA2::update(const void *data, std::size_t size)
{
	::EVP_DigestUpdate(m_ctx.get(), data, size);
}

std::size_t SHA2::finalize(unsigned char *out, std::size_t size)
{
	unsigned out_size{};
	std::array<unsigned char, EVP_MAX_MD_SIZE> hash{};
	::EVP_DigestFinal_ex(m_ctx.get(), &hash[0], &out_size);
	assert(out_size <= EVP_MAX_MD_SIZE);

	auto count = std::min(size, static_cast<std::size_t>(out_size));
	::memcpy(out, hash.data(), count);
	return count;
}

std::array<unsigned char, SHA2::size> SHA2::finalize()
{
	std::array<unsigned char, SHA2::size> result{};
	::EVP_DigestFinal_ex(m_ctx.get(), &result[0], nullptr);
	return result;
}

}} // end of namespace hrb
