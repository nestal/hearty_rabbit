/*
	Copyright © 2021 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/11/2021.
//

#include "HeartyRabbit.hh"
#include "crypto/Password.hh"
#include "crypto/Random.hh"
#include "net/Redis.hh"
#include "util/MMap.hh"

#include <nlohmann/json.hpp>
#include <fstream>

namespace hrb {

void HeartyRabbitServer::login(
	std::string username, const Password& password,
	std::function<void(std::error_code)> on_complete
)
{
	auto user_dir = m_root / username;

	std::error_code ec;
	auto passwd = MMap::open(user_dir/"passwd.json", ec);
	if (ec)
	{
		on_complete(ec);
		return;
	}

	if (Authentication::verify_password(nlohmann::json::parse(passwd.string()), password))
	{
		// Generate session ID and store it in database
		// Session ID must be generated by cryptographically secure random number generator.
		auto session = random_value<Authentication::SessionID>(system_random);

		m_redis->command(
			[completion = std::move(on_complete), this, session, username=std::move(username)](
				auto&&, std::error_code ec
			) mutable
			{
				// TODO: handle errors from "reply"
				m_self = Authentication{session, username};
				completion(ec);
			},

			"SET session:%b _%b EX %d",
			session.data(), session.size(),
			username.data(), username.size(),
			3600
		);
	}
}

std::error_code HeartyRabbitServer::add_user(std::string_view user_name, const Password& password)
{
	auto user_dir = m_root / user_name;

	std::error_code ec;
	std::filesystem::create_directories(user_dir, ec);
	if (ec)
		return ec;

	std::ofstream ofs{user_dir/"passwd.json"};
	ofs.exceptions(ofs.exceptions() | std::ios::failbit);

	try
	{
		ofs << Authentication::hash_password(password);
		return {};
	}
	catch (std::ios_base::failure& e)
	{
		return e.code();
	}
}

} // end of namespace hrb
