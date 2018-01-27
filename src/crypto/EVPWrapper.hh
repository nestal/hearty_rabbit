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
#include <openssl/evp.h>

namespace hrb {

struct HashCTXRelease
{
	void operator()(EVP_MD_CTX *ctx) const;
};
using HashCTX = std::unique_ptr<EVP_MD_CTX, HashCTXRelease>;

HashCTX NewHashCTX();

} // end of namespace hrb
