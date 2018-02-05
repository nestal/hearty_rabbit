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

#include <blake2.h>

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

	// 20 byte hash space should be large enough to avoid collision.
	// Hash values will be used as keys for database, so if they are
	// too long, searching the database will be too slow.
	static const std::size_t size = 20;

	void update(const void *data, std::size_t len);
	std::array<unsigned char, size> finalize();
	std::size_t finalize(unsigned char *out, std::size_t len);

private:
	::blake2b_state m_ctx{};
};

}} // end of namespace hrb::evp
