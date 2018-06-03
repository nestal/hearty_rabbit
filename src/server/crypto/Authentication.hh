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

#include "common/UserID.hh"

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
	using Cookie = UserID::Cookie ;

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

	const UserID& id() const {return m_uid;}
	const Cookie& cookie() const {return m_uid.cookie();}
	const std::string& user() const {return m_uid.user();}
	bool is_guest() const {return m_uid.is_guest();}

	bool operator==(const Authentication& rhs) const {return m_uid == rhs.m_uid;}
	bool operator!=(const Authentication& rhs) const {return m_uid != rhs.m_uid;}

private:
	void renew_session(
		redis::Connection& db,
		std::chrono::seconds session_length,
		std::function<void(std::error_code, Authentication&&)>&& completion
	) const;

private:
	static const std::string_view m_shared_auth_prefix;

private:
	UserID m_uid;
};

std::optional<Authentication::Cookie> parse_cookie(std::string_view cookie);

} // end of namespace
