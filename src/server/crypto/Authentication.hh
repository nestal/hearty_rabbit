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

#include "hrb/UserID.hh"
#include <nlohmann/json.hpp>

#include <array>
#include <chrono>
#include <functional>
#include <string_view>
#include <system_error>
#include <optional>

namespace hrb {

namespace redis {
class Connection;
}

class Password;
class Cookie;

class Authentication
{
public:
	using SessionID = std::array<unsigned char, 16>;

public:
	Authentication() = default;
	explicit Authentication(SessionID guest_session);
	Authentication(SessionID session, std::string_view user);
	Authentication(Authentication&&) = default;
	Authentication(const Authentication&) = default;
	~Authentication() = default;

	Authentication& operator=(Authentication&&) = default;
	Authentication& operator=(const Authentication&) = default;

	[[nodiscard]] bool is_valid() const noexcept;
	[[nodiscard]] Cookie cookie() const;
	[[nodiscard]] Cookie set_cookie(std::chrono::seconds session_length = std::chrono::seconds{3600}) const;
	[[nodiscard]] bool invariance() const;

	static void add_user(
		std::string_view username_mixed_case,
		const Password& password,
		redis::Connection& db,
		std::function<void(std::error_code)> completion
	);

	static nlohmann::json hash_password(const Password& password);
	static bool verify_password(const nlohmann::json& hash, const Password& password);
	void create_session(
		std::function<void(std::error_code)> completion,
		const std::string& username,
		redis::Connection& db,
		std::chrono::seconds session_length
	);

	static void verify_user(
		std::string_view username_mixed_case,
		Password&& password,
		redis::Connection& db,
		std::chrono::seconds session_length,
		std::function<void(std::error_code, Authentication&&)> completion
	);

	static void verify_session(
		const SessionID& cookie,
		redis::Connection& db,
		std::chrono::seconds session_length,
		std::function<void(std::error_code, Authentication&&)>&& completion
	);

	template <typename Complete, typename Duration>
	static void share_resource(
		std::string_view owner,
		std::string_view resource,
		Duration valid_period,
		redis::Connection& db,
		Complete&& comp
	);

	template <typename Complete>
	void is_shared_resource(
		std::string_view owner,
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

	[[nodiscard]] const UserID& id() const {return m_uid;}
	[[nodiscard]] const SessionID& session() const {return m_session;}

	bool operator==(const Authentication& rhs) const {return m_uid == rhs.m_uid;}
	bool operator!=(const Authentication& rhs) const {return m_uid != rhs.m_uid;}

	static std::optional<SessionID> parse_cookie(const Cookie& cookie);

private:
	void renew_session(
		redis::Connection& db,
		std::chrono::seconds session_length,
		std::function<void(std::error_code, Authentication&&)>&& completion
	) const;

private:
	static const std::string_view m_shared_auth_prefix;

private:
	UserID      m_uid;
	SessionID   m_session{};
};

} // end of namespace
