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

namespace hrb {

using HashCTX = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;

inline auto NewHashCTX()
{
	return HashCTX{EVP_MD_CTX_new(), EVP_MD_CTX_free};
}

} // end of namespace hrb
