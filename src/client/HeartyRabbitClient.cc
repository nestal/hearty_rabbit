/*
	Copyright © 2021 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 10/11/2021.
//

#include "HeartyRabbitClient.hh"

namespace hrb {

HeartyRabbitClient::HeartyRabbitClient(
	boost::asio::io_context& ioc,
	boost::asio::ssl::context& ctx,
	std::string_view host,
	std::string_view port
) :
	m_ioc{ioc}, m_ssl{ctx}, m_host{host}, m_port{port}
{
}

void HeartyRabbitClient::login(
	std::string_view user, const Password& password,
	std::function<void(std::error_code)> on_complete
)
{
/*	std::string username{user};

	// Launch the asynchronous operation
	auto req = std::make_shared<GenericHTTPRequest<http::string_body, http::string_body>>(m_ioc, m_ssl);
	req->init(m_host, m_port, "/login", http::verb::post);
	req->request().set(http::field::content_type, "application/x-www-form-urlencoded");
	req->request().body() = "username=" + username + "&password=" + std::string{password};

	req->on_load(
		[this, username, comp = std::forward<Complete>(comp)](auto ec, auto& req)
		{
			m_outstanding.finish(req.shared_from_this());

			if (req.response().result() == http::status::no_content)
			{
				m_cookie = req.response().at(http::field::set_cookie);
				m_user   = UserID{username};
			}
			else
				ec = Error::login_incorrect;

			comp(ec);
			req.shutdown();
		}
	);
	m_outstanding.add(std::move(req));
*/
}

void HeartyRabbitClient::verify_session(
	const SessionID& cookie,
	std::function<void(std::error_code)>&& completion
)
{
}

void HeartyRabbitClient::destroy_session(
	std::function<void(std::error_code)>&& completion
)
{
}

void HeartyRabbitClient::upload_file(
	const std::filesystem::directory_entry& local_file,
	const Path& remote_path,
	std::function<void(std::error_code)> on_complete
)
{
}

void HeartyRabbitClient::get_directory(
	const Path& path,
	std::function<void(Directory&& result, std::error_code)> on_complete
) const
{
}

/// \brief Get the io_context associated with this interface.
/// This io_context will be used to run the completion callbacks of the async operations.
/// \return
boost::asio::execution::any_executor<> HeartyRabbitClient::get_executor()
{
	return m_ioc.get_executor();
}

} // end of namespace hrb
