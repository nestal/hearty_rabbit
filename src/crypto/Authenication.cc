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
#include <random>

namespace {

union Salt
{
	std::array<std::uint64_t, 4> lls;
	std::array<char, 32>     ucs;
};
const int min_iteration = 5000;

} // end of anonymous namespace


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
	// There is no need to use cryptographically secure random number to generate the
	// salt because it is not a secret to the attacker.
	thread_local std::mt19937_64 salt_generator{secure_random<std::uint64_t>()};

	Salt salt;
	std::generate(salt.lls.begin(), salt.lls.end(), std::ref(salt_generator));

	auto key = password.derive_key({salt.ucs.data(), salt.ucs.size()}, min_iteration);

	db.command(
		[key, salt](auto reply, auto&& ec)
		{

		},
		"HSETNX user:%b salt %b key %b iteration %d",
		username.data(), username.size(),
		salt.ucs.data(), salt.ucs.size(),
		key.data(), key.size(),
		min_iteration
	);
}

} // end of namespace hrb
