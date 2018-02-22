/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/27/18.
//

#pragma once

#include "net/Redis.hh"

#include <array>
#include <functional>
#include <optional>
#include <string_view>
#include <system_error>

namespace hrb {

namespace redis {
class Connection;
}

class Password;

class Authentication
{
public:
	using Cookie = std::array<unsigned char, 16>;

	Authentication() = default;
	Authentication(Cookie cookie, std::string_view user) : m_cookie{cookie}, m_user{user} {}

	bool valid() const;

	static void add_user(
		std::string_view username_mixed_case,
		const Password& password,
		redis::Connection& db,
		std::function<void(std::error_code)> completion
	);

	static void verify_user(
		std::string_view username_mixed_case,
		Password&& password,
		redis::Connection& db,
		std::function<void(std::error_code, const Authentication&)> completion
	);

	template <typename Completion>
	static void verify_session(
		const Cookie& cookie,
		redis::Connection& db,
		Completion&& completion
	)
	{
		db.command(
			[comp=std::forward<Completion>(completion), cookie](redis::Reply reply, auto&& ec) mutable
			{
				comp(std::move(ec), Authentication{cookie, reply.as_string()});
			},
			"GET session:%b", cookie.data(), cookie.size()
		);
	}

	const Cookie& cookie() const {return m_cookie;}
	std::string_view user() const {return m_user;}

	template <typename Completion>
	static void destroy_session(
		const Authentication& auth,
		redis::Connection& db,
		Completion&& completion
	)
	{
		db.command(
			[comp=std::move(completion)](redis::Reply, auto&& ec) mutable
			{
				comp(std::move(ec));
			},
			"DEL session:%b", auth.m_cookie.data(), auth.m_cookie.size()
		);
	}

private:
	Cookie      m_cookie{};
	std::string m_user;
};

std::string set_cookie(const Authentication::Cookie& id);
std::optional<Authentication::Cookie> parse_cookie(std::string_view cookie);

} // end of namespace
