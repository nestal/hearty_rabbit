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
#include "util/Escape.hh"

#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string.hpp>

#include <openssl/crypto.h>
#include <blake2.h>

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

void verify_password(std::error_code& ec, const Password& password, const redis::Reply& reply)
{
	auto [salt, key, iter, hash_algorithm] = reply.as_tuple<4>(ec);
	if (!ec && salt.is_string() && key.is_string() && iter.is_string() && iter.to_int() > 0)
	{
		auto hash_algorithm_to_use = default_hash_algorithm;
		if (hash_algorithm)
			hash_algorithm_to_use = std::string{hash_algorithm.as_string()};

		auto pkey = password.derive_key(salt.as_string(), iter.to_int(), hash_algorithm_to_use);
		auto skey = key.as_string();
		if (pkey.size() != skey.size() || ::CRYPTO_memcmp(&pkey[0], &skey[0], pkey.size()) != 0)
		{
			ec = Error::login_incorrect;
		}
	}
	else if (!ec)
		ec = Error::login_incorrect;
}

template <typename Completion>
void generate_session_id(Completion&& completion, std::string_view username, redis::Connection& db)
{
	// Generate session ID and store it in database
	auto id = secure_random<SessionID>();
	db.command([completion = std::forward<Completion>(completion), id](redis::Reply, std::error_code ec)
	{
		// TODO: handle errors from "reply"
		completion(ec, id);
	}, "SETEX session:%b 3600 %b", id.data(), id.size(), username.data(), username.size());
}

} // end of anonymous namespace

void add_user(
	std::string_view username_mixed_case,
	const Password& password,
	redis::Connection& db,
	std::function<void(std::error_code)> completion
)
{
	auto salt = random_salt();
	auto key = password.derive_key({salt.data(), salt.size()}, min_iteration, default_hash_algorithm);

	// Convert username to lower case to ensure case-insensitive comparison.
	std::string username{username_mixed_case};
	boost::algorithm::to_lower(username);

	db.command(
		[completion=std::move(completion)](auto reply, auto&& ec)
		{
			if (!reply)
				ec = Error::redis_command_error;

			completion(std::move(ec));
		},
		"HMSET user:%b salt %b key %b iteration %d hash_algorithm %b",
		username.data(), username.size(),
		salt.data(), salt.size(),
		key.data(), key.size(),
		min_iteration,
		default_hash_algorithm.data(), default_hash_algorithm.size()
	);
}

void verify_user(
	std::string_view username_mixed_case,
	Password&& password,
	redis::Connection& db,
	std::function<void(std::error_code, const SessionID&)> completion
)
{
	// Convert username to lower case to ensure case-insensitive comparison.
	std::string username{username_mixed_case};
	boost::algorithm::to_lower(username);

	db.command(
		[
			db=db.shared_from_this(),
			username,
			password=std::move(password),
			completion=std::move(completion)
		](redis::Reply reply, auto&& ec)
		{
			// Verify password with the key in database
			if (!ec)
				verify_password(ec, password, reply);

			// Generate session ID and store it in database
			if (!ec)
				generate_session_id(std::move(completion), username, *db);

			else
				completion(std::move(ec), {});
		},
		"HMGET user:%b salt key iteration hash_algorithm",
		username.data(), username.size()
	);
}

void verify_session(
	const SessionID& id,
	redis::Connection& db,
	std::function<void(std::error_code, std::string_view)> completion
)
{
	db.command(
		[comp=std::move(completion)](redis::Reply reply, auto&& ec)
		{
			comp(std::move(ec), reply.as_string());
		},
		"GET session:%b", id.data(), id.size()
	);
}

void destroy_session(
	const SessionID& id,
	redis::Connection& db,
	std::function<void(std::error_code)> completion
)
{
	db.command(
		[comp=std::move(completion)](redis::Reply, auto&& ec)
		{
			comp(std::move(ec));
		},
		"DEL session:%b", id.data(), id.size()
	);
}

std::string set_cookie(const SessionID& id)
{
	std::string result = "id=";
	boost::algorithm::hex_lower(id.begin(), id.end(), std::back_inserter(result));
	result.append("; Secure; HttpOnly; SameSite=Strict; Max-Age=3600");
	return result;
}

std::optional<SessionID> parse_cookie(std::string_view cookie)
{
	while (!cookie.empty())
	{
		auto [name, match] = split_front(cookie, "=");
		auto value = (match == '=' ? std::get<0>(split_front(cookie, ";")) : std::string_view{});

		if (SessionID result{}; name == "id" && value.size() == result.size() * 2)
		{
			boost::algorithm::unhex(value.begin(), value.end(), result.begin());
			return result;
		}

		while (!cookie.empty() && cookie.front() == ' ')
			cookie.remove_prefix(1);
	}
	return std::nullopt;
}

} // end of namespace hrb
