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
#include "util/Magic.hh"

#include <nlohmann/json.hpp>
#include <cassert>
#include <fstream>

namespace hrb {

constexpr std::string_view passwd_file = "passwd.json";
constexpr std::string_view data_dir    = "data";

void HeartyRabbitServer::login(
	std::string_view username, const Password& password,
	std::function<void(std::error_code)> on_complete
)
{
	try
	{
		auto user_dir = m_root / username;

		std::ifstream ofs;
		ofs.exceptions(ofs.exceptions() | std::ios::failbit);
		ofs.open(user_dir/passwd_file);

		if (Authentication::verify_password(nlohmann::json::parse(ofs), password))
		{
			m_self.create_session(std::move(on_complete), std::string{username}, *m_redis, m_session_length);
		}
	}
	catch (std::ios_base::failure& e)
	{
		on_complete(e.code());
		return;
	}
}

std::error_code HeartyRabbitServer::add_user(std::string_view username, const Password& password)
{
	try
	{
		auto user_dir = m_root / username;

		std::filesystem::create_directories(user_dir);

		std::ofstream ofs{user_dir/passwd_file};
		ofs.exceptions(ofs.exceptions() | std::ios::failbit);
		ofs << Authentication::hash_password(password);
		return {};
	}
	catch (std::system_error& e)
	{
		return e.code();
	}
}

void HeartyRabbitServer::verify_session(
	const Authentication::SessionID& cookie,
	std::function<void(std::error_code)>&& completion
)
{
	Authentication::verify_session(cookie, *m_redis, m_session_length, [this, completion](auto ec, auto&& auth)
	{
		m_self = std::forward<decltype(auth)>(auth);
		completion(ec);
	});
}

void HeartyRabbitServer::get_directory(
	const Path& path,
	std::function<void(Directory&& result, std::error_code)> on_complete
) const
{
	try
	{
		auto relative_path = (path.is_absolute() ? path.relative_path() : path).lexically_normal();
		on_complete(
			Directory{std::filesystem::directory_entry{m_root / m_self.id().username() / data_dir / relative_path}},
			{}
		);
	}
	catch (std::system_error& e)
	{
		on_complete({}, e.code());
	}
}

void HeartyRabbitServer::upload_file(
	const std::filesystem::directory_entry& local_file,
	const Path& remote_path,
	std::function<void(std::error_code)> on_complete
)
{
	on_complete({});
}

DirectoryEntry::DirectoryEntry(const std::filesystem::directory_entry& physical_location) :
	m_filename{physical_location.path().filename()},
	m_mime{Magic::instance().mime(physical_location.path())},
	m_last_modified{std::chrono::file_clock::to_sys(physical_location.last_write_time())},
	m_permission{status(physical_location).permissions()}
{
}

} // end of namespace hrb
