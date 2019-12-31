/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/8/18.
//

#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

#include <openssl/evp.h>

int main( int argc, char* argv[] )
{
	OpenSSL_add_all_digests();

	return Catch::Session().run(argc, argv);
}