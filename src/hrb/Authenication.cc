/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/27/18.
//

#include "Authenication.hh"

#include "net/Redis.hh"
#include "util/Random.hh"
#include <openssl/evp.h>
#include <openssl/sha.h>

namespace hrb {

Authenication::Authenication(std::string_view username, std::string_view password, hrb::redis::Database& db)
{

}

void add_user(
	std::string_view username,
	std::string_view password,
	redis::Database& db,
	std::function<void(std::string_view cookie, std::error_code)> completion
)
{
	auto salt = random<32>();
	std::array<unsigned char, SHA512_DIGEST_LENGTH> key{};

	::PKCS5_PBKDF2_HMAC(
		password.data(),
		static_cast<int>(password.size()),
		salt.data(),
		static_cast<int>(salt.size()),
		5000,
		::EVP_sha512(),
		static_cast<int>(key.size()),
		&key[0]
	);

	std::string uname{username};

	db.command([uname, key, salt](auto reply, auto& ec)
	{

	}, "HSETNX user:%s username %s salt %b key %b");
}

} // end of namespace hrb
