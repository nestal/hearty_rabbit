/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/27/18.
//

#include "Authentication.hh"

#include "Password.hh"
#include "crypto/Random.hh"
#include "net/Redis.hh"
#include "util/Error.hh"
#include "util/Escape.hh"
#include "util/Log.hh"

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
		std::array<std::uint64_t, sizeof(Salt)/sizeof(std::uint64_t)>    lls;
		Salt salt;
	} tmp;

	// There is no need to use cryptographically secure random number to generate the
	// salt because it is not a secret to the attacker.
	static thread_local Blake2x salt_generator;
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
void create_session(
	Completion&& completion,
	const std::string& username,
	redis::Connection& db,
	std::chrono::seconds session_length
)
{
	// Generate session ID and store it in database
	// Session ID must be generated by cryptographically secure random number generator.
	// Need more testing on Blake2x before using it.
	Authentication auth{secure_random<Authentication::Cookie>(), username};
	db.command(
		[
			auth,
			completion = std::forward<Completion>(completion)
		](redis::Reply, std::error_code ec) mutable
		{
			// TODO: handle errors from "reply"
			completion(ec, std::move(auth));
		},

		"SET session:%b _%b EX %d",
		auth.cookie().data(), auth.cookie().size(),
		username.data(), username.size(),
		session_length.count()
	);
}

} // end of anonymous namespace

Authentication::Authentication(Cookie cookie, std::string_view user) :
	m_cookie{cookie}, m_user{user}
{
	if (m_user.empty())
		m_cookie = Cookie{};
}

void Authentication::add_user(
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

void Authentication::verify_session(
	const Cookie& cookie,
	redis::Connection& db,
	std::chrono::seconds session_length,
	std::function<void(std::error_code, Authentication&&)>&& completion
)
{
	static const char lua[] = "return {redis.call('GET', KEYS[1]), redis.call('TTL', KEYS[1])}";

	db.command([
			db=db.shared_from_this(),
			comp=std::move(completion),
			cookie, session_length
		](redis::Reply reply, auto&& ec) mutable
		{
			if (ec)
				return comp(ec, Authentication{});

			auto [user, ttl] = reply.as_tuple<2>(ec);
			if (ec || user.is_nil() || user.as_string().size() <= 1 || ttl.as_int() < 0)
				return comp(ec, Authentication{});

			Authentication auth{cookie, user.as_string().substr(1)};
			if (ttl.as_int() < session_length.count()/2)
			{
				Log(LOG_NOTICE, "%1% seconds before session timeout. Renewing session", ttl.as_int());
				auth.renew_session(*db, session_length, std::move(comp));
			}
			else
			{
				comp(ec, std::move(auth));
			}
		},
		"EVAL %s 1 session:%b", lua, cookie.data(), cookie.size()
	);
}

void Authentication::verify_user(
	std::string_view username_mixed_case,
	Password&& password,
	redis::Connection& db,
	std::chrono::seconds session_length,
	std::function<void(std::error_code, const Authentication&)> completion
)
{
	// Convert username to lower case to ensure case-insensitive comparison.
	std::string username{username_mixed_case};
	boost::algorithm::to_lower(username);

	db.command(
		[
			db=db.shared_from_this(),
			username, session_length,
			password=std::move(password),
			completion=std::move(completion)
		](redis::Reply reply, auto&& ec)
		{
			// Verify password with the key in database
			if (!ec)
				verify_password(ec, password, reply);

			// Generate session ID and store it in database
			if (!ec)
				create_session(std::move(completion), username, *db, session_length);

			else
				completion(std::move(ec), {});
		},
		"HMGET user:%b salt key iteration hash_algorithm",
		username.data(), username.size()
	);
}

std::string Authentication::set_cookie(std::chrono::seconds session_length) const
{
	std::string result = "id=";
	if (valid())
	{
		boost::algorithm::hex_lower(m_cookie.begin(), m_cookie.end(), std::back_inserter(result));
		result.append("; Secure; HttpOnly; SameSite=Strict; Path=/; Max-Age=");
		result.append(std::to_string(session_length.count()));
	}
	else
	{
		result.append("; Path=/; expires=Thu, Jan 01 1970 00:00:00 UTC");
	}
	return result;
}

std::optional<Authentication::Cookie> parse_cookie(std::string_view cookie)
{
	while (!cookie.empty())
	{
		auto [name, match] = split_left(cookie, "=");
		auto value = (match == '=' ? std::get<0>(split_left(cookie, ";")) : std::string_view{});

		if (Authentication::Cookie result{}; name == "id" && value.size() == result.size() * 2)
		{
			boost::algorithm::unhex(value.begin(), value.end(), result.begin());
			return result;
		}

		while (!cookie.empty() && cookie.front() == ' ')
			cookie.remove_prefix(1);
	}
	return std::nullopt;
}

void Authentication::destroy_session(
	redis::Connection& db,
	std::function<void(std::error_code)>&& completion
) const
{
	db.command(
		[comp=std::move(completion)](redis::Reply, auto&& ec) mutable
		{
			comp(std::move(ec));
		},
		"DEL session:%b", m_cookie.data(), m_cookie.size()
	);
}

bool Authentication::valid() const
{
	// if cookie is valid, user must not be empty
	// if cookie is invalid, user must be empty
	assert((m_cookie == Cookie{}) == m_user.empty() );

	return m_cookie != Cookie{} && !m_user.empty();
}

void Authentication::renew_session(
	redis::Connection& db,
	std::chrono::seconds session_length,
	std::function<void(std::error_code, Authentication&&)>&& completion
) const
{
	auto new_cookie = secure_random<Authentication::Cookie>();

	// Check if the old session is already renewed and renew it atomically.

	// If the value of the old session key start with an underscore
	// that means it has not been renewed.

	// When renewing, expire the old session cookie in 30 seconds.
	// This is to avoid session error for the pending requests in the
	// pipeline. These requests should still be using the old session.
	static const char lua[] = R"__(
		if string.sub(redis.call('GET', KEYS[1]), 1, 1) == '_'
		then
			redis.call('SET', KEYS[1], '#' .. ARGV[1], 'EX', 30)
			redis.call('SET', KEYS[2], '_' .. ARGV[1], 'EX', ARGV[2])
			return 1
		else
			return 0
		end
	)__";
	db.command(
		[comp=std::move(completion), *this, new_cookie](auto&& reply, auto ec)
		{
			// if error occurs or session already renewed, use the old session
			if (ec || reply.as_int() != 1)
			{
				Log(LOG_NOTICE, "user %1% session already renewed: %2%", m_user, ec);
				comp(ec, Authentication{*this});
			}
			else
			{
				Log(LOG_NOTICE, "user %1% session renewed successfully", m_user);
				comp(ec, Authentication{new_cookie, m_user});
			}
		},
		"EVAL %s 2 session:%b session:%b %b %d",
		lua,
		m_cookie.data(), m_cookie.size(),           // KEYS[1]: old session cookie
		new_cookie.data(), new_cookie.size(),       // KEYS[2]: new session cookie
		m_user.data(), m_user.size(),               // ARGV[1]: username
		session_length                              // ARGV[2]: session length
	);
}

} // end of namespace hrb
