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

#include "Password.hh"
#include "crypto/Random.hh"
#include "net/Redis.hh"
#include "util/Error.hh"

#include <openssl/evp.h>
#include <openssl/sha.h>

#include <random>

namespace hrb {
namespace {

using Salt = std::array<char, 32>;
Salt random_salt()
{
	union
	{
		std::array<std::uint64_t, 4>    lls;
		Salt salt;
	} tmp;

	// There is no need to use cryptographically secure random number to generate the
	// salt because it is not a secret to the attacker.
	static thread_local std::mt19937_64 salt_generator{secure_random<std::uint64_t>()};
	std::generate(tmp.lls.begin(), tmp.lls.end(), std::ref(salt_generator));
	return tmp.salt;
}

const int min_iteration = 5000;

} // end of anonymous namespace

void add_user(
	std::string_view username,
	const Password& password,
	redis::Database& db,
	std::function<void(std::error_code)> completion
)
{
	auto salt = random_salt();
	auto key = password.derive_key({salt.data(), salt.size()}, min_iteration);

	db.command(
		[completion=std::move(completion)](auto reply, auto&& ec)
		{
			if (!reply)
				ec = Error::redis_command_error;

			completion(ec);
		},
		"HMSET user:%b salt %b key %b iteration %d",
		username.data(), username.size(),
		salt.data(), salt.size(),
		key.data(), key.size(),
		min_iteration
	);
}

void verify_user(
	std::string_view username,
	Password&& password,
	redis::Database& db,
	std::function<void(std::error_code)> completion
)
{
	db.command(
		[password=std::move(password), completion=std::move(completion)](redis::Reply reply, auto&& ec)
		{
			if (!ec)
			{
				auto salt = reply.as_array(0).as_string();
				auto key  = reply.as_array(1).as_string();
				auto iter = reply.as_array(2).to_int();

				if (
					auto pkey = password.derive_key(salt, iter);
					salt.empty() || key.empty() || iter < min_iteration ||
					!std::equal(
						pkey.begin(), pkey.end(),
						key.begin(), key.end(),
						[](unsigned char p, char k){return p == static_cast<unsigned char>(k);}
					)
				)
				{
					ec = Error::login_incorrect;
				}
			}
			completion(ec);
		},
		"HMGET user:%b salt key iteration",
		username.data(), username.size()
	);
}

} // end of namespace hrb
