/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/7/18.
//

#include "Server.hh"

#include "SessionHandler.hh"

#include "crypto/Password.hh"
#include "crypto/Authentication.hh"

#include "util/Error.hh"
#include "util/Configuration.hh"
#include "util/Exception.hh"

#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/info.hpp>

#include <utility>

namespace hrb {

Server::Server(const Configuration& cfg) :
	m_cfg{cfg},
	m_ioc{static_cast<int>(std::max(1UL, cfg.thread_count()))},
	m_db{m_ioc, cfg.redis()},
	m_lib{cfg.web_root()},
	m_blob_db{cfg}
{
}

void Server::drop_privileges() const
{
	// drop privileges if run as root
	if (::getuid() == 0)
	{
		// must set group ID before setting user ID, otherwise we have no
		// priviledge to set group ID
		if (::setgid(m_cfg.group_id()) != 0)
			BOOST_THROW_EXCEPTION(hrb::SystemError()
				<< ErrorCode(std::error_code(errno, std::system_category()))
				<< boost::errinfo_api_function("setgid")
			);

		if (::setuid(m_cfg.user_id()) != 0)
			BOOST_THROW_EXCEPTION(hrb::SystemError()
				<< ErrorCode(std::error_code(errno, std::system_category()))
				<< boost::errinfo_api_function("setuid")
			);
	}

	if (::getuid() == 0)
		throw std::runtime_error("cannot run as root");
}

void Server::add_user(std::string_view username, Password&& password, std::function<void(std::error_code)> complete)
{
	Authentication::add_user(username, std::move(password), *m_db.alloc(), [&complete](std::error_code&& ec)
	{
		complete(std::move(ec));
	});
	m_ioc.run();
}

boost::asio::io_context& Server::get_io_context()
{
	return m_ioc;
}

SessionHandler Server::start_session()
{
	return {m_db.alloc(), m_lib, m_blob_db, m_cfg};
}


} // end of namespace
