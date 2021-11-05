/*
	Copyright © 2021 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/11/2021.
//

#pragma once

#include "crypto/Authentication.hh"
#include "net/Redis.hh"

#include <string_view>
#include <filesystem>

namespace hrb {

class Password;
class Cookie;

class HeartyRabbit
{
public:
	// If you don't login, you will be a guest.
	virtual void login(
		std::string username, const Password& password,
		std::function<void(std::error_code)> on_complete
	) = 0;

	[[nodiscard]] virtual const Authentication& auth() const = 0;
};

class HeartyRabbitServer : public HeartyRabbit
{
public:
	explicit HeartyRabbitServer(std::filesystem::path root, std::shared_ptr<redis::Connection> conn) :
		m_root{std::move(root)}, m_redis{std::move(conn)}
	{
	}

	std::error_code add_user(
		std::string_view user_name, const Password& password
	);

	void login(
		std::string username, const Password& password,
		std::function<void(std::error_code)> on_complete
	) override;

	[[nodiscard]] const Authentication& auth() const override {return m_self;}

private:
	std::filesystem::path               m_root;
	std::shared_ptr<redis::Connection>  m_redis;
	Authentication                      m_self;
};

} // end of namespace hrb
