/*
	Copyright © 2021 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 10/11/2021.
//

#pragma once

#include "HeartyRabbit.hh"

#include "Authentication.hh"
#include "net/Redis.hh"

namespace hrb {

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
		const SessionID& cookie,
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

	[[nodiscard]] const SessionID& auth() const override {return m_self.session();}
	[[nodiscard]] const UserID& user() const override {return m_self.id();}

	[[nodiscard]] boost::asio::execution::any_executor<> get_executor() override {return m_redis->get_executor();}

private:
	std::filesystem::path               m_root;
	std::shared_ptr<redis::Connection>  m_redis;
	Authentication                      m_self;

	// configurations
	std::chrono::seconds                m_session_length;
};


} // end of namespace hrb
