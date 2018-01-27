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
#include "crypto/Random.hh"
#include "Password.hh"
#include <openssl/evp.h>
#include <openssl/sha.h>

namespace hrb {

Authenication::Authenication(std::string_view username, std::string_view password, hrb::redis::Database& db)
{

}

void add_user(
	std::string_view username,
	const Password& password,
	redis::Database& db,
	std::function<void(std::string_view cookie, std::error_code)> completion
)
{
	auto salt = random<32>();
	auto key = password.derive_key({salt.data(), salt.size()}, 5000);

	db.command(
		[key, salt](auto reply, auto&& ec)
		{

		},
		"HSETNX user:%b salt %b key %b",
		username.data(), username.size(),
		salt.data(), salt.size(),
		key.data(), key.size()
	);
}

} // end of namespace hrb
