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

/// \brief Denote a path in the Hearty Rabbit database.
/// It is not used for representing paths in filesystems, which uses std::filesystem::path directly.
using Path = std::filesystem::path;

class DirectoryEntry
{
public:
	DirectoryEntry() = default;
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
	std::filesystem::perms                  m_permission{};
};

class Directory
{
public:
	Directory() = default;
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

/// \brief HeartyRabbit interface.
/// The interface for all HeartyRabbit operations are implemented by this interface. It is the same for
/// server and client. Most functions are async, which means that may not be completed upon return. When
/// the operation requested is completed, a callback will be called in the io_context associated with this
/// interface.
class HeartyRabbit
{
public:
	// If you don't login, you will be an anonymous user or a guest.
	virtual void login(
		std::string_view username, const Password& password,
		std::function<void(std::error_code)> on_complete
	) = 0;

	/// \brief Get back credentials after login.
	/// \return Login credentials: e.g. session ID and user name.
	[[nodiscard]] virtual const Authentication& auth() const = 0;

	virtual void verify_session(
		const Authentication::SessionID& cookie,
		std::function<void(std::error_code)>&& completion
	) = 0;

	virtual void destroy_session(
		std::function<void(std::error_code)>&& completion
	) = 0;

	/// \brief Upload a file.
	/// Copy a file from local filesystem into Hearty Rabbit. On the client, this function
	/// will upload the file by HTTP PUT to the server. On the server, this function will
	/// copy the uploaded temporary file to the user's data directory and update (remove)
	/// the cache.
	/// \param local_file   Must ex
	/// \param remote_path
	virtual void upload_file(
		const std::filesystem::directory_entry& local_file,
		const Path& remote_path,
		std::function<void(std::error_code)> on_complete
	) = 0;

	virtual void get_directory(
		const Path& path,
		std::function<void(Directory&& result, std::error_code)> on_complete
	) const = 0;

	/// \brief Get the io_context associated with this interface.
	/// This io_context will be used to run the completion callbacks of the async operations.
	/// \return
	[[nodiscard]] virtual boost::asio::execution_context& get_context() = 0;
};

class HeartyRabbitServer : public HeartyRabbit
{
public:
	explicit HeartyRabbitServer(
		std::filesystem::path root, std::shared_ptr<redis::Connection> conn,
		std::chrono::seconds session_length = std::chrono::seconds{3600}
	) :
		m_root{std::move(root)}, m_session_length{session_length}, m_redis{std::move(conn)}
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
		std::function<void(std::error_code)>&& completion
	) override;

	void destroy_session(
		std::function<void(std::error_code)>&& completion
	) override;

	void upload_file(
		const std::filesystem::directory_entry& local_file,
		const Path& remote_path,
		std::function<void(std::error_code)> on_complete
	) override;

	void get_directory(
		const Path& path,
		std::function<void(Directory&& result, std::error_code)> on_complete
	) const override;

	[[nodiscard]] const Authentication& auth() const override {return m_self;}

	[[nodiscard]] boost::asio::execution_context& get_context() override;

private:
	std::filesystem::path               m_root;
	std::shared_ptr<redis::Connection>  m_redis;
	Authentication                      m_self;

	// configurations
	std::chrono::seconds                m_session_length;
};

} // end of namespace hrb
