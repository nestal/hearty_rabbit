/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 6/5/18.
//

#include "ServerInstance.hh"

#include "hrb/Server.hh"
#include "util/Configuration.hh"

#include "common/FS.hh"

#include <cassert>
#include <thread>

namespace hrb {

struct ServerInstance::Impl
{
	Impl() :
		m_cfg{0, nullptr,
			(fs::path{__FILE__}.parent_path().parent_path().parent_path() / "etc" / "hearty_rabbit" / "hearty_rabbit.json").string().c_str()
		},
		m_srv{m_cfg}
	{
		m_srv.listen();
	}
	~Impl()
	{
		assert(m_thread.joinable());
		m_srv.get_io_context().stop();
		m_thread.join();
	}

	Configuration m_cfg;
	Server m_srv{m_cfg};

	std::thread m_thread;
};

ServerInstance::ServerInstance() : m_impl{std::make_unique<Impl>()} {}
ServerInstance::~ServerInstance() = default;

void ServerInstance::run()
{
	m_impl->m_thread = std::thread([this]{m_impl->m_srv.get_io_context().run();});
}

} // end of namespace hrb
