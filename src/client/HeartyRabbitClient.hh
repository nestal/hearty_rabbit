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

#include "http/RequestScheduler.hh"

#include "util/Cookie.hh"
#include "hrb/UserID.hh"
#include "util/FS.hh"

#include <boost/beast/http/verb.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/http/file_body.hpp>

#include <type_traits>

namespace hrb {

class HeartyRabbitClient : public HeartyRabbit
{
public:
	HeartyRabbitClient(
		boost::asio::io_context& ioc,
		boost::asio::ssl::context& ctx,
		std::string_view host,
		std::string_view port
	);

	void login(
		std::string_view username, const Password& password,
		std::function<void(std::error_code)> on_complete
	) override;

	[[nodiscard]] const Authentication& auth() const override {return m_auth;}

	void verify_session(
		const Authentication::SessionID& cookie,
		std::function<void(std::error_code)>&& completion
	) override;

	void destroy_session(
		std::function<void(std::error_code)>&& completion
	) override;

	/// \brief Upload a file.
	/// Copy a file from local filesystem into Hearty Rabbit. On the client, this function
	/// will upload the file by HTTP PUT to the server. On the server, this function will
	/// copy the uploaded temporary file to the user's data directory and update (remove)
	/// the cache.
	/// \param local_file   Must ex
	/// \param remote_path
	void upload_file(
		const std::filesystem::directory_entry& local_file,
		const Path& remote_path,
		std::function<void(std::error_code)> on_complete
	) override;

	void get_directory(
		const Path& path,
		std::function<void(Directory&& result, std::error_code)> on_complete
	) const override;

	/// \brief Get the io_context associated with this interface.
	/// This io_context will be used to run the completion callbacks of the async operations.
	/// \return
	[[nodiscard]] boost::asio::execution::any_executor<> get_executor() override;

private:
	// connection to the server
	boost::asio::io_context&    m_ioc;
	boost::asio::ssl::context&  m_ssl;

	// configurations
	std::string m_host;
	std::string m_port;

	// authenticated user
	Authentication  m_auth;

	// outstanding and pending requests
	RequestScheduler m_outstanding;
};

} // end of namespace hrb
