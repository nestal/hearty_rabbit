/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/28/18.
//

#pragma once

#include <openssl/evp.h>
#include <openssl/sha.h>
#include <memory>

namespace hrb {
namespace evp {

class SHA3
{
public:
	SHA3();
	~SHA3() = default;

	static const std::size_t size = SHA512_DIGEST_LENGTH;

	void update(const void *data, std::size_t size);

	template <std::size_t N>
	std::size_t finalize(unsigned char (&out)[N])
	{
		unsigned out_size{N};
		if constexpr (N >= EVP_MAX_MD_SIZE)
		{
			::EVP_DigestFinal_ex(m_ctx.get(), &out[0], &out_size);
			return out_size;
		}
		else
			return finalize(out, N);
	}

	std::array<unsigned char, size> finalize();
	std::size_t finalize(unsigned char *out, std::size_t size);

private:
	struct Deleter
	{
		void operator()(EVP_MD_CTX *ctx) const;
	};

	std::unique_ptr<EVP_MD_CTX, Deleter> m_ctx;
};

}} // end of namespace hrb::evp
