/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/4/18.
//

#pragma once

#include <openssl/sha.h>
#include <array>

namespace hrb {

static const std::size_t object_id_size  = SHA224_DIGEST_LENGTH;
static constexpr std::array<unsigned char, 5> object_redis_key_prefix = {'b','l','o','b', ':'};
using ObjectRedisKey = std::array<unsigned char, object_id_size + object_redis_key_prefix.size()>;

} // end of namespace
