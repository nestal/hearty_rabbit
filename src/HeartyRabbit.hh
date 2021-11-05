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

#include <chrono>
#include <string_view>
#include <filesystem>

namespace hrb {

class Password;
class Cookie;

class DirectoryEntry
{
public:
	DirectoryEntry(
		std::string filename,
		std::string mime,
		std::chrono::system_clock::time_point mtime,
		std::filesystem::perms perms
	) :
		m_filename{std::move(filename)},
		m_mime{std::move(mime)},
		m_last_modified{mtime},
		m_permission{perms}
	{
	}

	explicit DirectoryEntry(const std::filesystem::directory_entry& physical_location);

	[[nodiscard]] auto& filename() const {return m_filename;}
	[[nodiscard]] auto& mime() const {return m_mime;}
	[[nodiscard]] auto last_modified() const {return m_last_modified;}
	[[nodiscard]] auto permission() const {return m_permission;}

private:
	std::string                             m_filename;
	std::string                             m_mime;
	std::chrono::system_clock::time_point   m_last_modified;
	std::filesystem::perms                  m_permission;
};

class Directory
{
public:
	explicit Directory(const std::filesystem::directory_entry& physical_location) : m_self{physical_location}
	{
		std::filesystem::directory_iterator iterator{physical_location.path()};
		for (auto& child : iterator)
			m_children.emplace_back(child);
	}

private:
	DirectoryEntry  m_self;
	std::vector<DirectoryEntry> m_children;
};

class HeartyRabbit
{
public:
	// If you don't login, you will be a guest.
	virtual void login(
		std::string_view username, const Password& password,
		std::function<void(std::error_code)> on_complete
	) = 0;

	[[nodiscard]] virtual const Authentication& auth() const = 0;
	[[nodiscard]] virtual Directory get_directory(const std::filesystem::path& path) const = 0;
};

class HeartyRabbitServer : public HeartyRabbit
{
public:
	explicit HeartyRabbitServer(std::filesystem::path root, std::shared_ptr<redis::Connection> conn) :
		m_root{std::move(root)}, m_redis{std::move(conn)}
	{
	}

	std::error_code add_user(
		std::string_view username, const Password& password
	);

	void login(
		std::string_view username, const Password& password,
		std::function<void(std::error_code)> on_complete
	) override;

	void verify_session(
		const Authentication::SessionID& cookie,
		std::chrono::seconds session_length,
		std::function<void(std::error_code)>&& completion
	);

	[[nodiscard]] Directory get_directory(const std::filesystem::path& path) const override;

	[[nodiscard]] const Authentication& auth() const override {return m_self;}

private:
	std::filesystem::path               m_root;
	std::shared_ptr<redis::Connection>  m_redis;
	Authentication                      m_self;
};

} // end of namespace hrb
