/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/28/18.
//

#include "EVPWrapper.hh"

namespace hrb {

#if OPENSSL_VERSION_NUMBER < 0x1010000fL
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

void HashCTXRelease::operator()(EVP_MD_CTX *ctx) const
{
	// This is a macro, so can't take its address and put it to unique_ptr
	::EVP_MD_CTX_destroy(ctx);
}

HashCTX NewHashCTX()
{
	return HashCTX{EVP_MD_CTX_new(), HashCTXRelease{}};
}

} // end of namespace hrb
