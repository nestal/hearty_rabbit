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
#include "crypto/Password.hh"
#include "crypto/Random.hh"
#include "util/Configuration.hh"

#include "util/FS.hh"

#include "config.hh"

#include <cassert>
#include <thread>

namespace hrb {

struct ServerInstance::Impl
{
	Impl() :
		m_srv{m_cfg}
	{
		Server::add_user(m_cfg, "sumsum", Password{"bearbear"}, [](auto){});
		m_srv.listen();
	}
	~Impl()
	{
		assert(m_thread.joinable());
		m_srv.get_io_context().stop();
		m_thread.join();
	}

	static const Configuration m_cfg;
	Server m_srv{m_cfg};

	std::thread m_thread;
};

const Configuration ServerInstance::Impl::m_cfg{[]()
{
	fs::path path = fs::path{CONFIG_FILE} / hrb::constants::config_filename;
	Configuration cfg{
		0, nullptr,
		path.string().c_str()
	};

	// use a randomize port
	auto https_port = cfg.listen_https().port();
	auto http_port  = cfg.listen_http().port();
	while (https_port <= 1024 || https_port == cfg.listen_https().port())
		randomize(https_port);
	while (http_port <= 1024 || http_port == cfg.listen_http().port())
		randomize(http_port);
	cfg.change_listen_ports(https_port, http_port);

	return cfg;
}()};


ServerInstance::ServerInstance() : m_impl{std::make_unique<Impl>()} {}
ServerInstance::~ServerInstance() = default;

void ServerInstance::run()
{
	m_impl->m_thread = std::thread([this]{m_impl->m_srv.get_io_context().run();});
}

std::string ServerInstance::listen_https_port()
{
	return std::to_string(Impl::m_cfg.listen_https().port());
}

} // end of namespace hrb
