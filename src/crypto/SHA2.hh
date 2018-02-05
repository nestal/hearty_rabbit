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

#include <memory>

namespace hrb {
namespace evp {

class SHA2
{
public:
	SHA2();
	SHA2(SHA2&&) = default;
	SHA2(const SHA2&) = delete;
	~SHA2() = default;

	SHA2& operator=(SHA2&&) = default;
	SHA2& operator=(const SHA2&) = delete;

	static const std::size_t size = 28;

	void update(const void *data, std::size_t size);
	std::array<unsigned char, size> finalize();
	std::size_t finalize(unsigned char *out, std::size_t size);

private:
	struct Deleter
	{
		void operator()(void *ctx) const noexcept ;
	};

	std::unique_ptr<void, Deleter> m_ctx;
};

}} // end of namespace hrb::evp
