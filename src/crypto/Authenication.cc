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
const std::string default_hash_algorithm = "sha512";

} // end of anonymous namespace

void add_user(
	std::string_view username,
	const Password& password,
	redis::Database& db,
	std::function<void(std::error_code)> completion
)
{
	auto salt = random_salt();
	auto key = password.derive_key({salt.data(), salt.size()}, min_iteration, default_hash_algorithm);

	db.command(
		[completion=std::move(completion)](auto reply, auto&& ec)
		{
			if (!reply)
				ec = Error::redis_command_error;

			completion(ec);
		},
		"HMSET user:%b salt %b key %b iteration %d hash_algorithm %b",
		username.data(), username.size(),
		salt.data(), salt.size(),
		key.data(), key.size(),
		min_iteration,
		default_hash_algorithm.c_str(), default_hash_algorithm.size()
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
				auto [salt, key, iter, hash_algorithm] = reply.as_tuple<4>(ec);
				if (!ec && salt.is_string() && key.is_string() && iter.is_string() && iter.to_int() > 0)
				{
					auto hash_algorithm_to_use = default_hash_algorithm;
					if (hash_algorithm)
						hash_algorithm_to_use = std::string{hash_algorithm.as_string()};

					auto pkey = password.derive_key(salt.as_string(), iter.to_int(), hash_algorithm_to_use);
					auto skey = key.as_string();
					if (!std::equal(
						pkey.begin(), pkey.end(),
						skey.begin(), skey.end(),
						[](unsigned char p, char k)
						{ return p == static_cast<unsigned char>(k); }
					))
					{
						ec = Error::login_incorrect;
					}
				}
				else if (!ec)
					ec = Error::login_incorrect;
			}
			completion(ec);
		},
		"HMGET user:%b salt key iteration hash_algorithm",
		username.data(), username.size()
	);
}

} // end of namespace hrb
