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

#include <array>
#include <memory>

namespace hrb {

class Blake2
{
public:
	Blake2();
	Blake2(Blake2&&) = default;
	Blake2(const Blake2& src) : Blake2()
	{
		::EVP_MD_CTX_copy_ex(m_ctx.get(), src.m_ctx.get());
	}
	~Blake2() = default;

	Blake2& operator=(Blake2&&) = default;
	Blake2& operator=(const Blake2& src)
	{
		auto copy{src};
		std::swap(m_ctx, copy.m_ctx);
		return *this;
	}

	// 20 byte hash space should be large enough to avoid collision.
	// Hash values will be used as keys for database, so if they are
	// too long, searching the database will be too slow.
	static const std::size_t size = 20;

	void update(const void *data, std::size_t len);
	std::array<unsigned char, size> finalize();
	std::size_t finalize(unsigned char *out, std::size_t len);

private:
	struct Deleter {void operator()(::EVP_MD_CTX*);};
	std::unique_ptr<::EVP_MD_CTX, Deleter> m_ctx{::EVP_MD_CTX_new()};
};

} // end of namespace hrb::evp
