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

#include <array>
#include <functional>
#include <optional>
#include <string_view>
#include <system_error>
#include <chrono>

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
	Authentication(Cookie cookie, std::string_view user, bool guest=false);
	Authentication(Authentication&&) = default;
	Authentication(const Authentication&) = default;
	~Authentication() = default;

	Authentication& operator=(Authentication&&) = default;
	Authentication& operator=(const Authentication&) = default;

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
		std::chrono::seconds session_length,
		std::function<void(std::error_code, const Authentication&)> completion
	);

	static void verify_session(
		const Cookie& cookie,
		redis::Connection& db,
		std::chrono::seconds session_length,
		std::function<void(std::error_code, Authentication&&)>&& completion
	);

	template <typename Complete>
	static void share_resource(
		std::string_view owner,
		std::string_view resource,
		redis::Connection& db,
		Complete&& comp
	);

	template <typename Complete>
	void is_shared_resource(
		std::string_view resource,
		redis::Connection& db,
		Complete&& comp
	);

	template <typename Complete>
	static void list_guests(
		std::string_view owner,
		std::string_view resource,
		redis::Connection& db,
		Complete&& comp
	);

	void destroy_session(
		redis::Connection& db,
		std::function<void(std::error_code)>&& completion
	) const;

	// TODO: update UT so that we don't need a default argument
	std::string set_cookie(std::chrono::seconds session_length = std::chrono::seconds{3600}) const;

	const Cookie& cookie() const {return m_cookie;}
	const std::string& user() const {return m_user;}
	bool is_guest() const {return m_guest;}

	bool operator==(const Authentication& rhs) const;
	bool operator!=(const Authentication& rhs) const;

private:
	void renew_session(
		redis::Connection& db,
		std::chrono::seconds session_length,
		std::function<void(std::error_code, Authentication&&)>&& completion
	) const;

private:
	static const std::string_view m_shared_auth_prefix;

private:
	Cookie      m_cookie{};
	std::string m_user;
	bool        m_guest{false};
};

std::optional<Authentication::Cookie> parse_cookie(std::string_view cookie);

} // end of namespace
